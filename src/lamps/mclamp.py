# An example of a simple lamp on standard housing hardware with all basic behaviors enabled
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import Behaviour
import uasyncio as asyncio

simple = StandardLamp(name="mclamp", base_color="70964b", shade_color="#ffffff")

class Social(Behaviour):
    async def arrivals(self):
        while True:
            arrived = await self.lamp.network.arrived()

            previous_shade = list(self.lamp.base.pixels)
            print("%s has arrived" % (arrived["name"]))
            await self.lamp.base.fade((arrived["base_color"] * self.lamp.base.num_pixels), 90)
            await asyncio.sleep(50)
            await self.lamp.base.fade(previous_shade, 40)

    async def run(self):
        asyncio.create_task(self.arrivals())

#simple.add_behaviour(Social)
simple.wake()
