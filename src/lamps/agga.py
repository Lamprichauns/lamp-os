# Agga is a lamp shaped like a dragon egg
import uasyncio as asyncio
import utime
from machine import Pin, PWM
from components.network.access_point import AccessPoint
from lamp_core.standard_lamp import StandardLamp
from utils.color_tools import darken, brighten
from utils.gradient import create_gradient
from behaviours.configurator import Configurator, configurator_load_data
from behaviours.social import SocialGreeting
from behaviours.lamp_brightness import LampBrightness
from behaviours.lamp_dmx import LampDmx
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.easing import pingpong_ease
from utils.fade import pingpong_fade
from random import choice, randrange

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 40, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 40, "color":"#ff7a00", "pin": 14 },
    "lamp": { "name": "agga", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-agga" },
    "dmx": { "channel": 4 }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
configurable.base.num_pixels = 40
configurable.base.default_pixels = create_gradient((18, 0, 0, 0), (106, 0, 0, 10), steps=17) + create_gradient((106, 0, 0, 10), (18, 0, 0, 0), steps=18) + [(0,0,0,0)] * 5
configurable.shade.default_pixels = [(255,47,11,20)] * configurable.shade.num_pixels

class EveningSky(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.cloud_positions = []
        self.cloud_brightness = 10
        self.cloud_color = (240, 0, 230, 240)
        self.cloud_style = 1
        self.auto_play = True

    async def draw(self):
        if self.cloud_style == 0:
            percentage = pingpong_ease(0, self.cloud_brightness, 0, self.frames, self.frame)

            for j in self.cloud_positions:
                self.lamp.base.buffer[j] = brighten(self.lamp.base.buffer[j], percentage)

        else:
            for k in self.cloud_positions:
                self.lamp.base.buffer[k] = pingpong_fade(self.lamp.base.buffer[k], self.cloud_color, self.lamp.base.buffer[k], self.frames, self.frame)

        await self.next_frame()

    async def control(self):
        while True:
            if self.frame == 0:
                self.cloud_brightness = choice(range(145, 185))
                self.cloud_positions = [randrange(10, 24, 1) for i in range(1)]
                self.cloud_style = choice(range(0, 1))
            await asyncio.sleep(0)

class RandomMovement(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.cloud_positions = []
        self.cloud_brightness = 10
        self.cloud_color = (240, 0, 230, 240)
        self.cloud_style = 1
        self.next_play = 0

    async def draw(self):
        if self.cloud_style == 0:
            percentage = pingpong_ease(0, self.cloud_brightness, 0, self.frames, self.frame)

            for j in self.cloud_positions:
                self.lamp.base.buffer[j] = brighten(self.lamp.base.buffer[j], percentage)

        else:
            for k in self.cloud_positions:
                self.lamp.base.buffer[k] = pingpong_fade(self.lamp.base.buffer[k], self.cloud_color, self.lamp.base.buffer[k], self.frames, self.frame)

        await self.next_frame()

    async def control(self):
        while True:
            if self.animation_state not in (AnimationState.PLAYING, AnimationState.STOPPING):
                if utime.ticks_ms() > self.next_play and choice(range(0, 50)) >= 48:
                    self.cloud_brightness = choice(range(50, 220))
                    self.cloud_positions = [randrange(10, 24, 1) for i in range(1)]
                    self.cloud_style = choice(range(0, 1))
                    self.next_play = utime.ticks_ms() + randrange(200, 10000)
                    self.play()
                    self.stop()

            await asyncio.sleep(0)

configurable.add_behaviour(EveningSky(configurable, frames=500, auto_play=True))
configurable.add_behaviour(RandomMovement(configurable, frames=20))
configurable.add_behaviour(LampDmx(configurable, frames=30, auto_play=True))
configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
