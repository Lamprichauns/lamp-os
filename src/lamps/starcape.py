import uasyncio as asyncio
from random import randrange, choice
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import AnimatedBehaviour
from utils.easing import ease
from utils.color_tools import brighten
from utils.fade import pingpong_fade
from components.network.access_point import AccessPoint
from behaviours.lamp_fade_in import LampFadeIn
from behaviours.lamp_idle import LampIdle
from behaviours.social import SocialGreeting
config = {
    "shade": { "pin": 13, "pixels": 12 },
    "lamp": { "name": "starcape", "default_behaviours": "false" },
    "wifi": { "ssid": "lamp-290712" }
}

# the cape uses small fiber optics that slightly shimmer over any color
class Shimmer(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.pattern = [0]*self.lamp.shade.num_pixels
        for i in range(self.lamp.shade.num_pixels):
            self.pattern[i] = ease(0, 20, self.lamp.shade.num_pixels, i)

    async def draw(self):
        self.pattern = self.pattern[3:] + self.pattern[:3]

        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = brighten(self.lamp.shade.buffer[i], self.pattern[i])

        if self.is_last_frame():
            val = choice(range(10, 30))
            for i in range(self.lamp.shade.num_pixels):
                self.pattern[i] = ease(0, val, self.lamp.shade.num_pixels, i)

        await self.next_frame()


# sweep across the fibers with a bluer light every so often
class Spin(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.current = 0

    async def draw(self):
        transition_color = (self.lamp.shade.buffer[self.current][0], self.lamp.shade.buffer[self.current][1], 255, 255)
        self.lamp.shade.buffer[self.current] = pingpong_fade(self.lamp.shade.buffer[self.current], transition_color, self.lamp.shade.buffer[self.current], self.frames, self.frame)

        if self.is_last_frame():
            self.current +=1

            if self.current > self.lamp.shade.num_pixels-1:
                self.current = 0

        await self.next_frame()

starcape = StandardLamp(name="starcape", base_color="#126674", shade_color="#5A1700", config_opts=config)
starcape.shade.default_pixels = [(150, 40, 0, 150)] * starcape.shade.num_pixels

starcape.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
starcape.add_behaviour(LampFadeIn(starcape, frames=30, chained_behaviors = [LampIdle, Spin, Shimmer]))
starcape.add_behaviour(LampIdle(starcape, frames=1))
starcape.add_behaviour(SocialGreeting(starcape, frames=300))
starcape.add_behaviour(Spin(starcape, frames=200))
starcape.add_behaviour(Shimmer(starcape, frames=30))
starcape.wake()
