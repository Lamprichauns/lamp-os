from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

lamp = Lamp("gramp", "#66ff00", "#ffffff")

# Gramp has a narrow neck and we need to shut some of these pixels off 
def knock_out_neck_pixels(pixels): 
    for i in range(23,30):
        pixels[i] = (0,4,0,0)
    return pixels

default_pixels = knock_out_neck_pixels(lamp.base.default_pixels)

# Modify some of the colors at the top to make it purty
for i in range(30,34):
    default_pixels[i] = (5,20,0,0)
for i in range(6):
    default_pixels[34 + i] = ((100 + i * 10),150,0,0)

lamp.base.default_pixels = default_pixels

class GlitchyGramp(Behaviour):
    async def glitch(self):
        print("Glitching")        
        lamp = self.lamp

        colors = list(lamp.base.pixels)
        glitch_color = random.choice([ 
            (0,0,0,255,0),
            (0,0,0,50,0),
            (0,0,0,100,0),
            (250,250,250,0),
            (100,100,150,0)
        ])
        gc = knock_out_neck_pixels([glitch_color] * 40)
        await lamp.base.until_colors_changed(gc)
        await asyncio.sleep_ms(random.choice([30,20000,120,800]))
        await lamp.base.until_colors_changed(colors)

    async def run(self):
        while True:
            await asyncio.sleep(random.choice(range(3200)))
            await self.glitch()    

class ShiftyGramp(Behaviour):
    async def shift(self):
        print("Shifting")
        lamp = self.lamp 

        options = list(range(len(self.palettes)))
        options.remove(self.active_palette)
        choice = random.choice(options)

        dest_colors = knock_out_neck_pixels(self.palettes[choice]) 

        await lamp.base.until_faded_to(dest_colors,500,50)
        self.active_palette = choice
        print("shifted!")

    async def run(self):
        self.active_palette = 0

        self.palettes = {}
        self.palettes[0] = self.lamp.base.default_pixels
        self.palettes[1] = [(100,200,0,0)] * 40
        self.palettes[2] = [(0,160,0,40)] * 40
        self.palettes[3] = [(30,80,5,0)] * 40                
        self.palettes[3] = [(70,200,5,90)] * 40   

        while True:
            await asyncio.sleep(random.choice(range(1600)))
            await self.shift()   

lamp.add_behaviour(ShiftyGramp)
lamp.add_behaviour(GlitchyGramp)
lamp.wake()
