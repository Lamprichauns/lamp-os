# A barebones lamp with configuration and social features
import random
import uasyncio as asyncio
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_in import LampFadeIn
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import AnimatedBehaviour
from utils.fade import fade
from utils.gradient import create_gradient
import random

# Override the standard lamp configs if necessary
config = {
    "base":  { "pin": 13 },
    "shade": { "pin": 12 },
    #"lamp":  { "default_behaviours": False }
}
# Sarah Tonin's lamp - simlar to Trans, but diff colors
lamp = StandardLamp("tonin", "#9A0E82", "#9A0E82", config)

# Set the default pixels to a mixture of blue and purple
pixels = lamp.base.default_pixels

for i in [20,27,29,30,32,33,34,36,38,39]:
    pixels[i] = (255, 100, 100, 0)

for i in [22, 10, 15, 14, 13]:
    pixels[i] = (108, 129, 232, 0)

#pixels[25] = (0,40,240,0)
#pixels[23] = (100,40,240,0)

lamp.base.default_pixels = pixels

class ColorShuffle(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.new_pixels = []
        self.current_pixels = []

    async def draw(self):
        if self.new_pixels:
            for i in range(self.lamp.base.num_pixels):
                self.lamp.base.buffer[i] = fade(self.current_pixels[i], self.new_pixels[i], self.frames, self.frame)
            if self.is_last_frame():
                self.new_pixels = []
                self.current_pixels = self.new_pixels.copy()
        else:
            # Keep current pixels
            self.lamp.base.buffer = self.current_pixels.copy()

        await self.next_frame()

    async def control(self):
        while True:
            self.current_pixels = list(self.lamp.base.default_pixels).copy()
            self.new_pixels = sorted(self.current_pixels, key=lambda x: random.random())

            print("Shuffling pixels")
            await asyncio.sleep(30) #300

#lamp.add_behaviour(LampFadeIn(lamp, frames=30, chained_behaviors=[ColorShuffle]))
#lamp.add_behaviour(ColorShuffle(lamp, frames=32)) #32000
lamp.add_behaviour(SocialGreeting(lamp, frames=3000))
lamp.wake()
