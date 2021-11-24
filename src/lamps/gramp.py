from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

lamp = Lamp("gramp", "#00ff00", "#ffffff")


class GlitchyGramp(Behaviour):
    def __init__(self,lamp):
        self.lamp = lamp

    async def glitch(self):
        lamp = self.lamp

        asyncio.create_task(lamp.base.set_color((0,0,0,255)))
        await asyncio.sleep_ms(random.choice([10,5,80,300,800]))
        asyncio.create_task(lamp.base.reset())

    async def run(self):
        while True:
            await asyncio.sleep(random.choice(range(3600)))
            await asyncio.create_task(self.glitch())       

lamp.add_behaviour(GlitchyGramp)


lamp.wake()