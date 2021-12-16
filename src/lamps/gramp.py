from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

lamp = Lamp("gramp", "#66ff00", "#ffffff")


class GlitchyGramp(Behaviour):
    def __init__(self,lamp):
        self.lamp = lamp

    async def glitch(self):
        lamp = self.lamp

        await lamp.base.until_color_changed((0,0,0,255))
        await asyncio.sleep_ms(random.choice([10,5,80,300]))
        await lamp.base.until_reset()

    async def run(self):
        while True:
            await asyncio.sleep(random.choice(range(1800)))
            await self.glitch()    

lamp.add_behaviour(GlitchyGramp)

lamp.wake()