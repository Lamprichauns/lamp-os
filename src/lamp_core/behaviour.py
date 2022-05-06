import uasyncio as asyncio

# Behaviours make up the asynchronous tasks performed by the lamp
class Behaviour:
    def __init__(self, lamp):
        self.lamp = lamp

    def __str__(self):
        return self.__class__.__name__

    async def run(self):
        pass

class AnimationStyle:
    REPEAT = 1
    PING_PONG = 2

# An animation behavior that will loop indefinitely
class AnimatedBehaviour(Behaviour):
    def __init__(self, lamp, frames=60, animate=AnimationStyle.REPEAT):
        super().__init__(lamp)
        self.animate = animate
        self.frames = frames
        self.frame = 0
        self.direction = True

    async def next_frame(self):
        self.frame += 1
        if self.frame >= self.frames:
            self.frame = 0
            if self.animate == AnimationStyle.PING_PONG:
                self.direction = not self.direction
        await asyncio.sleep(0)

# A behaviour that happens exactly one time at startup
class StartupBehaviour(Behaviour):
    pass
