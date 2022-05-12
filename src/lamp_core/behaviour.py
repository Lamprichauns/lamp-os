import gc
import uasyncio as asyncio
import utime

# Behaviours make up the asynchronous tasks performed by the lamp
class Behaviour:
    def __init__(self, lamp):
        self.lamp = lamp

    def __str__(self):
        return self.__class__.__name__

    async def run(self):
        pass

class AnimationState:
    # Animation is playing normally
    PLAYING = 1

    # Animation is about to stop dead in its tracks
    PAUSING = 2

    # Animation will no longer contribute pixels to the scene and will keep
    # its playhead at the last frame position
    PAUSED = 3

    # Animation will stop gracefully and let the total frame count continue
    # to contribute to the scene
    STOPPING = 4

    # Animation will no longer contribute to the scene and it will
    # resume from the beginning of the frame count
    STOPPED = 5

# An animation behavior that will loop indefinitely
class AnimatedBehaviour(Behaviour):
    def __init__(self, lamp, frames=60, chained_behaviors=None):
        super().__init__(lamp)
        self.frames = frames
        self.frame = 0
        self.current_loop = 0
        self.animation_state = AnimationState.STOPPED
        self.chained_behaviors = chained_behaviors if isinstance(chained_behaviors, list) else []
        gc.collect()

    async def control(self):
        pass

    async def draw(self):
        pass

    async def animate(self):
        while True:
            if self.animation_state in(AnimationState.PAUSED, AnimationState.STOPPED):
                await self.next_frame()
                continue

            await self.draw()

    async def run(self):
        await asyncio.gather(
            asyncio.create_task(self.control()),
            asyncio.create_task(self.animate())
        )

    def pause(self):
        if self.animation_state == AnimationState.PLAYING:
            self.animation_state = AnimationState.PAUSING

    def stop(self):
        if self.animation_state == AnimationState.PLAYING:
            self.animation_state = AnimationState.STOPPING

    def play(self):
        self.animation_state = AnimationState.PLAYING

    async def next_frame(self):
        if self.animation_state == AnimationState.PAUSING:
            self.animation_state = AnimationState.PAUSED

        if self.animation_state not in (AnimationState.PAUSED, AnimationState.STOPPED):
            self.frame += 1
            gc.collect()

        if self.frame >= self.frames:
            self.frame = 0
            self.current_loop += 1

            if self.animation_state == AnimationState.STOPPING:
                self.animation_state = AnimationState.STOPPED

        await asyncio.sleep(0)

class BackgroundBehavior(Behaviour):
    pass

class DrawBehaviour(Behaviour):
    async def run(self):
        self.lamp.base.fill((0, 0, 0, 0))
        self.lamp.shade.fill((0, 0, 0, 0))
        ticks = 0
        avg_duration = 0
        while True:
            t = utime.ticks_us()
            self.lamp.base.flush()
            self.lamp.shade.flush()

            await asyncio.sleep(0)

            avg_duration += utime.ticks_diff(utime.ticks_us(), t)
            if ticks % 60 == 0:
                if self.lamp.debug is True:
                    print('Average Draw Duration = {:6.3f}ms'.format(avg_duration/60000))
                    print('Framerate: {}Hz'.format(1000/(avg_duration/60000)))
                    # pylint: disable=no-member
                    print('Memory: {}, Free: {}'.format(gc.mem_alloc(), gc.mem_free()))
                avg_duration = 0
            ticks += 1
