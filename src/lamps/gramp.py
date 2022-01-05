from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

config = { 
    "base": { "pin": 15, "pixels": 40}, 
    "shade": { "pin": 14, "pixels": 40}, 
    "touch": { "pin": 32 }
}

gramp = Lamp("gramp", "#40b000", "#ffffff", config)

# Gramp has a narrow neck and we need to shut some of these pixels off 
def knock_out_neck_pixels(pixels): 
    for i in range(22,34):
        pixels[i] = (0,1,0,0)
    return pixels

default_pixels = knock_out_neck_pixels(gramp.base.default_pixels)

# Modify some of the colors at the top to make it purty
for i in range(30,34):
    default_pixels[i] = (5,15,0,0)

for i in range(6):
    default_pixels[34 + i] = ((100 + (i * 10)),150,0,0)
    default_pixels[12] = (0,0,30,0)
    default_pixels[22-i] = ((5 + (i * 10)),50,0,0)

gramp.base.default_pixels = default_pixels

class GlitchyGramp(Behaviour):
    async def glitch(self,max=5000):
        colors = list(self.lamp.base.pixels)        
        glitch_time_options = [time for time in [30,10,15,5,5,20,120,250,500,650,800,5000] if time <= max]     

        glitch_color = random.choice([ 
            (0,0,0,80,0),
            (255,255,255,0),
            (0,0,80,50,0),
            (0,160,0,150)
        ])

        glitch_pause = random.choice(glitch_time_options)

        self.lamp.base.fill(knock_out_neck_pixels([glitch_color] * 40))
        await asyncio.sleep_ms(glitch_pause)

        self.lamp.base.fill(colors)

    async def run(self):
        # Occasionally glitch a couple times and then end in default colors. If mid-shift, cancel that.
        while True:         
            await asyncio.sleep(random.choice(range(300,2700)))
            async with self.lamp.lock:
                print("Glitching")                
                await self.lamp.behaviour(ShiftyGramp).abort()

                for i in range(1,2): await self.glitch(500)
                await self.glitch() # possible long glitch
                self.lamp.base.fill(self.lamp.base.default_pixels)

class ShiftyGramp(Behaviour):
    async def shift(self):
        await self.abort()

        options = list(range(len(self.palettes)))
        options.remove(self.active_palette)

        choice = random.choice(options)
        dest_colors = knock_out_neck_pixels(self.palettes[choice]) 

        print("Shifting to %s" % (choice))
   
        # Do the shift in the background, it takes a long time
        self.task = asyncio.create_task(self.lamp.base.async_fade(dest_colors,400,200))
        self.active_palette = choice

    async def abort(self): 
        if self.task: 
            print("Aborting shift")
            self.task.cancel() 
            self.task = None

    async def unshift(self):
        print("Reverting to default colors")

        async with self.lamp.lock:
            await self.abort()            
            await self.lamp.behaviour(GlitchyGramp).glitch(200)
            self.lamp.base.fill(self.lamp.base.default_pixels)

    async def run(self):
        self.active_palette = 0
        self.task = None

        self.palettes = {}
        self.palettes[0] = [(90,240,0,0)] * 40
        self.palettes[1] = [(0,250,0,80)] * 40     
        self.palettes[2] = [(10,160,10,0)] * 40       
        self.palettes[3] = [(30,100,5,40)] * 40    
        self.palettes[4] = [(20,250,40,0)] * 40   
        self.palettes[5] = self.lamp.base.default_pixels.copy()
        self.palettes[6] = [(0,250,30,0)] * 40

        # some tweaks 
        for i in range(10):
            self.palettes[5][i] = ((100 + (i * 10)),100,0,0)
            self.palettes[3][i+10] = (30,120 + (i * 10),5,20)    

        for i in range(6):
            self.palettes[6][34 + i] = ((10 + (i * 10)),250,30,0)
  

        while True:
            # Every now and then, shift to a new palette
            await asyncio.sleep(random.choice(range(120,1200)))
            await self.shift()   

            # Revert back to default after awhile
            await asyncio.sleep(random.choice(range(300,900)))
            await self.unshift()

class TouchyGramp(Behaviour):
    # This is mostly to flesh out some various ideas/interface bits. 
    # Dim while being touched - block other behaviours while being touched. 
    # when releasing: 
    # - fire glitch behaviour
    # - return to previous colors
    # - fire shift behaviour
    async def touched(self):
        dim_pixels = knock_out_neck_pixels([(0,10,0,0)] * 40)

        previous_shade = list(self.lamp.shade.pixels)        
        previous_base = list(self.lamp.base.pixels)
    
        self.lamp.shade.fill((150,40,0,0))
        self.lamp.base.fill(dim_pixels)

        while self.lamp.touch.is_touched():            
            self.lamp.shade.fill((int(self.lamp.touch.value()),10,0,0))
            asyncio.sleep_ms(500)

        self.lamp.shade.fill(previous_shade)
        self.lamp.base.fill(previous_base)

        print("Touch released")   

        await self.lamp.behaviour(GlitchyGramp).glitch(200)
        await self.lamp.behaviour(ShiftyGramp).shift()     

    async def run(self):
        while True: 
            await asyncio.sleep_ms(100)  

            if self.lamp.touch.is_touched():
                async with self.lamp.lock:
                    print("Touched - %s (calibrated at %s)" % (self.lamp.touch.value(), self.lamp.touch.average()))
                    await self.touched()

class SocialGramp(Behaviour):
    async def blink(self,color,pausetime): 
        async with self.lamp.lock:
            previous_base = list(self.lamp.base.pixels)
            self.lamp.base.fill(knock_out_neck_pixels([color] * 40))
            await asyncio.sleep_ms(pausetime)
            self.lamp.base.fill(previous_base)

    async def arrivals(self):
        while True:
            arrived = await self.lamp.network.arrived()
            print("%s has arrived" % (arrived["name"]))
            await self.blink(arrived["base_color"],500)
            await self.blink(arrived["base_color"],500)            

    async def departures(self):
        while True:
            departed = await self.lamp.network.departed()
            print("%s has departed" % (departed["name"]))
            await self.blink(departed["base_color"],1000)
            
    async def run(self):
        asyncio.create_task(self.arrivals())
        asyncio.create_task(self.departures())


gramp.add_behaviour(TouchyGramp)
gramp.add_behaviour(ShiftyGramp)
gramp.add_behaviour(GlitchyGramp)  
gramp.add_behaviour(SocialGramp)  

gramp.wake()