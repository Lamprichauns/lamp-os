from lamp_core.behaviour import AnimatedBehaviour

class LampIdle(AnimatedBehaviour):
    async def draw(self):
        self.lamp.shade.buffer = self.lamp.shade.default_pixels.copy()
        self.lamp.base.buffer = self.lamp.base.default_pixels.copy()

        await self.next_frame()
