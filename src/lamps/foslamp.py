from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

config = {
    "base": { "pin": 2, "pixels": 40},
    "shade": { "pin": 12, "pixels": 40},
    "touch": { "pin": 32 }
}
foslamp = Lamp("foslamp", "#88ff22", "#ffffff", config)
pixels = foslamp.base.default_pixels

for i in [20,27,29,30,32,33,34,36,38,39]:
    pixels[i] = (0,118,187,0)

for i in [22, 10, 15, 14, 13]:
    pixels[i] = (200,50,190, 0)

pixels[25] = (18,40,240,0)
pixels[23] = (255,0,1620,0)

foslamp.base.default_pixels = pixels


# Move the pink stars around, very slowly.
class ShuffleStars(Behaviour):
    async def move(self):
        new_pixels = self.lamp.base.default_pixels
        new_pixels = sorted(new_pixels, key=lambda x: random.random())

        print("Slowly moving stars")
        await self.lamp.base.async_fade(new_pixels, 300, 1000) # 5 min fade time
        print("Done.")

    async def run(self):
       while True:
            await asyncio.sleep(300)
            async with self.lamp.lock:
                await self.move()



foslamp.add_behaviour(ShuffleStars)


foslamp.wake()
