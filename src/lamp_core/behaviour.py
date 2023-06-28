import gc
import uasyncio as asyncio
import utime

# Behaviours make up the asynchronous tasks performed by the lamp
class Behaviour:
    def __init__(self, lamp):
        self.lamp = lamp
        self.use_in_home_mode = True

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
# frames = animations run at 30fps max, so divide the time by that number
# frame = the current frame of the animation
# current_loop = a counter of the number of loops played
# chained_behaviors = play other behaviors when the controller is finished its work
# immediate_control = True to start the control block immediately. defaults to 5 seconds after wake
class AnimatedBehaviour(Behaviour):
    def __init__(self, lamp, frames=60, chained_behaviors=None, auto_play=False):
        super().__init__(lamp)
        self.frames = frames
        self.frame = 0
        self.current_loop = 0
        self.chained_behaviors = chained_behaviors if isinstance(chained_behaviors, list) else []
        self.immediate_control = False
        if auto_play is True:
            self.animation_state = AnimationState.PLAYING
        else:
            self.animation_state = AnimationState.STOPPED

        gc.collect()

    async def control(self):
        pass

    async def draw(self):
        pass

    async def animate(self):
        while True:
            if self.animation_state == AnimationState.STOPPED:
                await self.next_frame()
                continue

            await self.draw()

    async def run_control(self):
        if self.immediate_control:
            await self.control()
        else:
            await asyncio.sleep(5)
            await self.control()

    async def run(self):
        await asyncio.gather(
            asyncio.create_task(self.run_control()),
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

    def reset(self):
        self.frame = 0

    def is_last_frame(self):
        return self.frame == self.frames-1

    async def next_frame(self):
        if self.animation_state == AnimationState.PAUSING:
            self.animation_state = AnimationState.PAUSED

        if self.animation_state not in (AnimationState.PAUSED, AnimationState.STOPPED):
            self.frame += 1

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
        last_duration = 0
        avg_duration = 0
        while True:
            t = utime.ticks_ms()
            self.lamp.base.flush()
            self.lamp.shade.flush()
            gc.collect()

            # Add a framerate cap to save power in light loads
            wait_ms = 33-last_duration
            await asyncio.sleep(0)
            last_duration = utime.ticks_diff(utime.ticks_ms(), t)

            if wait_ms > 0:
                utime.sleep_ms(wait_ms)

            avg_duration += utime.ticks_diff(utime.ticks_ms(), t)
            if ticks % 60 == 0:
                if self.lamp.debug is True:
                    if avg_duration//60 > 0:
                        print('Average Draw Duration = {}ms'.format(avg_duration//60))
                        print('Framerate: {}Hz'.format(1000//(avg_duration//60)))
                        # pylint: disable=no-member
                        print('Memory: {}, Free: {}'.format(gc.mem_alloc(), gc.mem_free()))
                avg_duration = 0
            ticks += 1
