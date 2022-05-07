import uasyncio as asyncio

# Behaviours make up the asynchronous tasks performed by the lamp
class Behaviour:
    def __init__(self, lamp):
        self.lamp = lamp

    def __str__(self):
        return self.__class__.__name__

    async def run(self):
        pass

class AnimationState:
    PLAYING = 1
    PAUSING = 2
    PAUSED = 3

# An animation behavior that will loop indefinitely
class AnimatedBehaviour(Behaviour):
    def __init__(self, lamp, frames=60):
        super().__init__(lamp)
        self.frames = frames
        self.frame = 0
        self.animation_state = AnimationState.PLAYING

    async def animate(self):
        while True:
            if self.animation_state == AnimationState.PAUSED:
                await self.next_frame()
                continue

            await self.run()

    def pause(self):
        if self.animation_state == AnimationState.PLAYING:
            self.animation_state = AnimationState.PAUSING

    def resume(self):
        self.animation_state = AnimationState.PLAYING

    async def next_frame(self):
        if self.animation_state != AnimationState.PAUSED:
            self.frame += 1

        if self.frame >= self.frames:
            self.frame = 0

            if self.animation_state == AnimationState.PAUSING:
                self.animation_state = AnimationState.PAUSED

        await asyncio.sleep(0)

# A behaviour that happens exactly one time at startup
class StartupBehaviour(Behaviour):
    pass

# A controller handler
class ControllerBehaviour(Behaviour):
    pass
