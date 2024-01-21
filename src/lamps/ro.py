# A barebones lamp with configuration and social features
import random
import uasyncio as asyncio
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_in import LampFadeIn
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import AnimatedBehaviour
from utils.fade import fade
from utils.gradient import create_gradient

# Override the standard lamp configs if necessary
config = {
    "base":  { "pin": 12, "pixels": 151 },
    "shade": { "pin": 14, "pixels": 151 },
    "lamp":  { "default_behaviours": False }
}
lamp = StandardLamp("ro", "#ff0000", "#d94f00", config)


class SocialStack(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.lamp_count = 0
        self.segments = 1
        self.lamp_colors = []

    async def draw(self):
        await self.next_frame()

        for i in self.lamp_count: 
            for pixel in range(i, i+self.segment_pixels): 
                self.lamp.shade.buffer[pixel] = fade(self.current_pixels[i], self.lamp_colors[i], self.frames, self.frame)

    async def control(self):
        # wait for lamp to be started up for a while on first boot
        await asyncio.sleep(15)

        while True:
            self.lamp_colors = []
            self.lamp_count = len(self.lamp.network)
            self.segment_pixels = round(150/self.lamp_count)

            for name, data in self.lamps.network.items():
                self.lamp_colors.append(data["base_color"])

            print("%s lamps" % self.lamp_count)
            print("%s segments" % self.segments)

            await asyncio.sleep(15)            


class ColorFade(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.current_palette = 0
        self.previous_palette = 0
        self.palette_change = False

        self.palettes = [
            create_gradient((207, 4, 112, 0), (255, 0, 64, 0), self.lamp.base.num_pixels),
            create_gradient((48, 255, 62, 0), (156, 152, 37, 0), self.lamp.base.num_pixels),
            create_gradient((27, 20, 163, 0), (17, 24, 115, 0), self.lamp.base.num_pixels),
            create_gradient((190, 27, 227, 0), (163, 3, 27, 0), self.lamp.base.num_pixels),
            create_gradient((193,34,1950, 0), (58,45,253, 0), self.lamp.base.num_pixels),
            create_gradient((131,58,180,0), (252,176,69, 0), self.lamp.base.num_pixels),    
            create_gradient((195,139,34,0), (253,45,175, 0), self.lamp.base.num_pixels),    
        ]

    async def draw(self):
        if self.palette_change is True:
            for i in range(self.lamp.base.num_pixels):
                self.lamp.base.buffer[i] = fade(self.palettes[self.previous_palette][i], self.palettes[self.current_palette][i], self.frames, self.frame)

            if self.is_last_frame():
                self.palette_change = False
        else:
            self.lamp.base.buffer = self.palettes[self.current_palette].copy()

        await self.next_frame()

    async def control(self):
        # wait for lamp to be started up for a while on first boot
        await asyncio.sleep(15)

        while True:
            if self.palette_change is True:
                await asyncio.sleep(0)
                continue

            await asyncio.sleep(random.choice(range(100,1800)))

            palette_options = list(range(len(self.palettes)))
            palette_options.remove(self.current_palette)
            choice = random.choice(palette_options)

            self.frame = 0
            self.palette_change = True
            self.previous_palette = self.current_palette
            self.current_palette = choice

            print("Changing colors to palette %s" % (choice))


lamp.add_behaviour(LampFadeIn(lamp, frames=30, chained_behaviors=[ColorFade]))
lamp.add_behaviour(ColorFade(lamp, frames=20000))
#lamp.add_behaviour(SocialStack(lamp, frames=300))
lamp.add_behaviour(SocialGreeting(lamp, frames=2000))
lamp.wake()
