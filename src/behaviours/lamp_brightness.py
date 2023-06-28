# Handle basic lamp brightness controls
import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour
from utils.color_tools import darken

class LampBrightness(AnimatedBehaviour):
    def __init__(self, *args, brightness=100, **kwargs):
        super().__init__(*args, **kwargs)
        self.immediate_control = True
        self.amount = 100-brightness

    async def draw(self):
        if self.amount == 0:
            pass
        else:
            for i in range(self.lamp.base.num_pixels):
                self.lamp.base.buffer[i] = darken(self.lamp.base.buffer[i], self.amount)

            for i in range(self.lamp.shade.num_pixels):
                self.lamp.shade.buffer[i] = darken(self.lamp.shade.buffer[i], self.amount)

        await self.next_frame()

    async def control(self):
        self.play()
        await asyncio.sleep(0)
