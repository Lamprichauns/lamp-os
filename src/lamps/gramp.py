from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random


config = { 
    "base": { "pin": 15, "pixels": 40}, 
    "shade": { "pin": 14, "pixels": 40}, 
    "touch": { "pin": 32 }
}

lamp = Lamp("gramp", "#40b000", "#ffffff", config)

# Gramp has a narrow neck and we need to shut some of these pixels off 
def knock_out_neck_pixels(pixels): 
    for i in range(22,34):
        pixels[i] = (0,0,0,0)
    return pixels

default_pixels = knock_out_neck_pixels(lamp.base.default_pixels)

# Modify some of the colors at the top to make it purty
for i in range(30,34):
    default_pixels[i] = (5,15,0,0)

for i in range(6):
    default_pixels[34 + i] = ((100 + (i * 10)),150,0,0)
    default_pixels[12] = (0,0,30,0)

lamp.base.default_pixels = default_pixels

class GlitchyGramp(Behaviour):
    async def glitch(self):
        print("Glitching")        
        lamp = self.lamp

        colors = list(lamp.base.pixels)
        glitch_color = random.choice([ 
            (0,0,0,80,0),
            (0,0,0,50,0),
            (0,160,0,150),
            (250,250,250,0),
            (70,60,150,0)
        ])

        await lamp.base.until_colors_changed(knock_out_neck_pixels([glitch_color] * 40))
        await asyncio.sleep_ms(random.choice([30,5,20,250,500,120,800,5000]))
        await lamp.base.until_colors_changed(colors)

    async def run(self):
        while True:
            next_glitch = random.choice(range(300,2700))         
            await asyncio.sleep(next_glitch)
            for i in range(1,4): 
                await self.glitch()

class ShiftyGramp(Behaviour):
    async def shift(self):
        print("Shifting")
        lamp = self.lamp 

        options = list(range(len(self.palettes)))
        options.remove(self.active_palette)
        choice = random.choice(options)
        dest_colors = knock_out_neck_pixels(self.palettes[choice]) 

        await lamp.base.until_faded_to(dest_colors,400,20)
        self.active_palette = choice
        print("shifted to %s" % (choice))

    async def unshift(self):
        await lamp.base.until_faded_to(self.lamp.base.default_pixels,400,150)
        print("reverted to default")        

    async def run(self):
        self.active_palette = 0

        self.palettes = {}
        self.palettes[0] = [(90,240,0,0)] * 40
        self.palettes[1] = [(0,250,0,80)] * 40     
        self.palettes[2] = [(10,160,10,0)] * 40       
        self.palettes[3] = [(30,100,5,40)] * 40    
        self.palettes[4] = [(20,250,40,0)] * 40   
        self.palettes[5] = self.lamp.base.default_pixels.copy()

        # some tweaks 
        for i in range(10):
            self.palettes[5][i] = ((100 + (i * 10)),100,0,0)
            self.palettes[3][i+10] = (30,120 + (i * 10),5,20)       

        while True:
            # Wait between 5 and 60 min and then shift to a different palette
            await asyncio.sleep(10) #random.choice(range(300,3600)))
            await self.shift()   

            # Stay with this palette between 5 to 10 min, then return to defaults
            #await asyncio.sleep(random.choice(range(300,600)))
            #await self.unshift()

class TouchyGramp(Behaviour):
    async def touched(self):
        dim_pixels = knock_out_neck_pixels([(0,10,0,0)] * 40)

        previous_base = list(lamp.base.pixels)
        previous_shade = list(lamp.shade.pixels)
        
        await lamp.base.until_colors_changed(dim_pixels)
        await lamp.shade.until_color_changed((150,40,0,0))

        while lamp.touch.is_touched():            
            asyncio.sleep_ms(100)

        await lamp.base.until_colors_changed(previous_base)
        await lamp.shade.until_colors_changed(previous_shade)

    async def run(self):
        while True: 
            await asyncio.sleep_ms(100)  

            if lamp.touch.is_touched():
                print("Touched")
                async with lamp.lock:
                    await self.touched()


# :TODO: Implement a semaphore or something along those lines
# to allow behaviours to await other behaviours 

lamp.add_behaviour(TouchyGramp)
lamp.add_behaviour(ShiftyGramp)
lamp.add_behaviour(GlitchyGramp)  

lamp.wake()
