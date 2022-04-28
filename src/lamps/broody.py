from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

config = {
    "base": { "pin": 12, "pixels": 40},
    "shade": { "pin": 13, "pixels": 40},
    "touch": { "pin": 33 }
}
thislamp = Lamp("broody", "#953553", "#ffffff", config)
pixels = thislamp.base.default_pixels

for i in [34,35,36,38,39]:
    pixels[i] = (50,50,250,0)

for i in [1, 2, 3, 4, 5, 6]:
    pixels[i] = (255,30,0,0) #255,153,51

thislamp.base.default_pixels = pixels

class ShuffleColors(Behaviour):
    async def move(self):
        new_pixels = self.lamp.base.default_pixels
        new_pixels = sorted(new_pixels, key=lambda x: random.random())

        print("Shuffling colors")
        await self.lamp.base.async_fade(new_pixels, 300, 20) # 5 min fade time
        print("Done.")

    async def run(self):
       while True:
            #await asyncio.sleep(300)
            async with self.lamp.lock:
                await self.move()

class Social(Behaviour):
    async def arrivals(self):
        while True:
            arrived = await self.lamp.network.arrived()

            async with self.lamp.base.lock:
                print("%s has arrived" % (arrived["name"]))
                await self.lamp.shade.async_fade((arrived["base_color"] * self.lamp.base.num_pixels), 90)
                await asyncio.sleep(20)
                await self.lamp.shade.async_fade((0,0,0,255),40)

    async def run(self):
        asyncio.create_task(self.arrivals())

thislamp.add_behaviour(Social)
thislamp.add_behaviour(ShuffleColors)
thislamp.wake()
