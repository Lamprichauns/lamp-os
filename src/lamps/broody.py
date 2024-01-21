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
    "base":  { "pin": 12 },
    "shade": { "pin": 14 },
    "lamp":  { "default_behaviours": False }
}
lamp = StandardLamp("brody", "#ff0000", "#d94f00", config)

class ColorFade(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.current_palette = 0
        self.previous_palette = 0
        self.palette_change = False

        self.palettes = [
            [(255, 0, 0, 0)] * self.lamp.base.num_pixels,
            [(255, 100, 0, 0)] * self.lamp.base.num_pixels,
            [(255, 0, 23, 0)] * self.lamp.base.num_pixels,
            create_gradient((174, 5, 100, 0), (174, 0, 0, 0), self.lamp.base.num_pixels),
            [(138, 1, 7, 0)] * self.lamp.base.num_pixels,
            create_gradient((88, 1, 200, 0), (94, 3, 36, 0), self.lamp.base.num_pixels),
            create_gradient((40, 61, 50, 0), (255, 0, 0, 0), self.lamp.base.num_pixels),
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
lamp.add_behaviour(SocialGreeting(lamp, frames=3000))
lamp.wake()
