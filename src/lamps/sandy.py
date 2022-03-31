from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

config = { 
    "base": { "pin": 12, "pixels": 40}, 
    "shade": { "pin": 13, "pixels": 40}, 
    "touch": { "pin": 32 }
} 
andy = Lamp("andy", "#c800c8", "#ffffff", config)
pixels = andy.base.default_pixels 

for i in [20,27,29,30,32,33,34,36,38,39]:
    pixels[i] = (0,118,187,0)

for i in [22, 10, 15, 14, 13]:
    pixels[i] = (0,50,190, 0)

pixels[25] = (0,40,240,0)
pixels[23] = (255,0,1620,0)

andy.base.default_pixels = pixels  


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

class SuperNova(Behaviour):
    async def trigger(self):
        print("Triggering nova")              
        pixel = random.choice(range(0,self.lamp.base.num_pixels-10))
        pixel2 = random.choice(range(0,self.lamp.base.num_pixels-10))

        current_pixels = list(self.lamp.base.pixels)

        new_pixels = current_pixels.copy()

        # Slowly shift to orange
        new_pixels[pixel] = (255,0,187,0) 
        new_pixels[pixel2] = (255,0,187,0) 
        await self.lamp.base.async_fade(new_pixels,200,5)

        # Quick flash to white
        new_pixels[pixel] = (250,250,255,0) 
        new_pixels[pixel2] = (250,250,255,0) 
        await self.lamp.base.async_fade(new_pixels,80,2)
        
        # Quick flash to black
        new_pixels[pixel] = (0,0,0,0) 
        new_pixels[pixel2] = (0,0,0,0) 
        await self.lamp.base.async_fade(new_pixels,100,5)

        # Slow fade back to before
        await self.lamp.base.async_fade(current_pixels,200,5)
            
    async def run(self):
        while True:
            async with self.lamp.lock:
                await self.trigger()
            
            await asyncio.sleep(random.choice(range(30,600)))


andy.add_behaviour(ShuffleStars)
andy.add_behaviour(SuperNova)

andy.wake()