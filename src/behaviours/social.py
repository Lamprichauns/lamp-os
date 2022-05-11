import uasyncio as asyncio
from lamp_core.behaviour import Behaviour

class SocialGreeting(Behaviour):
    async def arrivals(self):
        while True:
            arrived = await self.lamp.network.arrived()

            async with self.lamp.shade.lock:
                print("%s has arrived" % (arrived["name"]))
                await self.lamp.shade.fade((arrived["base_color"] * self.lamp.base.num_pixels), 90)
                await asyncio.sleep(20)
                await self.lamp.shade.fade((0,0,0,255),50)

    async def run(self):
        asyncio.create_task(self.arrivals())
