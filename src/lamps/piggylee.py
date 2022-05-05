import random
import uasyncio as asyncio
from lamp_core.behaviour import Behaviour
from lamp_core.standard_lamp import StandardLamp

# This lamp is a hand soldered prototype so the pins are different.
config = {
    "base": { "pin": 2, "pixels": 40},
    "shade": { "pin": 26, "pixels": 40},
}
lamp = StandardLamp("piggylee", "#F23591", "#ffffff")

class SocialColors(Behaviour):
    def shade_color(self, color):
        pixels = [(0,0,0,255)] * 40

        for i in range(17,40):
            pixels[i] = color
        return pixels

    async def arrivals(self):
        while True:
            arrived = await self.lamp.network.arrived()

            async with self.lamp.base.lock:
                print("%s has arrived" % (arrived["name"]))
                await self.lamp.shade.fade(self.shade_color(arrived["base_color"]),100)
                self.current_shade_source = arrived["name"]

    async def departures(self):
        while True:
            departed = await self.lamp.network.departed()
            print("%s has departed" % (departed["name"]))

            if self.current_shade_source == departed["name"]:
                print("Lost current source, changing shade color")
                async with self.lamp.base.lock:
                    if self.lamp.network.lamps == {}:
                        new_color = (0,0,0,255)
                    else:
                        newsource = random.choice(list(self.lamp.network.lamps.keys()))
                        new_color = self.lamp.network.lamps[newsource]["base_color"]
                        self.current_shade_source = newsource
                    await self.lamp.shade.fade(self.shade_color(new_color),100)

    async def run(self):
        self.current_shade_source = None

        asyncio.create_task(self.arrivals())
        asyncio.create_task(self.departures())


lamp.add_behaviour(SocialColors)
lamp.wake()
