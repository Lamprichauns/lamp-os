# An example of a lamp that can be setup using a web app
from random import choice
import random
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.fade import fade
from utils.gradient import create_gradient
from behaviours.configurator import Configurator, configurator_load_data
from behaviours.lamp_brightness import LampBrightness
from lamp_core.frame_buffer import FrameBuffer
from utils.hex_to_rgbw import hex_to_rgbw
from components.led.neopixel import NeoPixel, ColorOrder
from utils.color_tools import darken
from behaviours.social import SocialGreeting

def calculate_vibrance(r, g, b):
    return (
        (max(r, g) - min(r, g)) +
        (max(g, b) - min(g, b)) +
        (max(b, r) - min(b, r))
    ) / 3

def generate_color():
    r = random.random()
    g = random.random()
    b = random.random()

    max_val = max(r, g, b)
    r = r / max_val
    g = g / max_val
    b = b / max_val

    vibrance = calculate_vibrance(r, g, b)
    count = 0

    # SETTING for color vibrance constraint
    min_vibrance = round(random.random() * 0.7) + 0.3

    while vibrance < min_vibrance and count < 50:
        count += 1
        r = r * r
        g = g * g
        b = b * b
        vibrance = calculate_vibrance(r, g, b)

    # SETTING for color brightness constraint
    scale = random.random() * 50

    r = round(r * (scale + 20) * 1.2)
    g = round(g * (scale + 20) * 0.9)
    b = round(b * (scale + 20) * 0.75)

    return (r, g, b, 0)  # default color

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 35, "color":"#36000F", "pin": 12 },
    "strip1": { "pixels": 30, "color":"#111111", "pin": 14 }, #uv
    "strip2": { "pixels": 100, "color":"#001C18", "pin": 27 }, #rgb
    "base": { "pixels": 0, "color":"#300783", "pin": 32 },
    "lamp": { "name": "configurable", "default_behaviours": False, "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-configurable" }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])
stray = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
stray.strip1 = FrameBuffer(hex_to_rgbw(config["strip1"]["color"]), config["strip1"]["pixels"], NeoPixel(config["strip1"]["pin"], config["strip1"]["pixels"], 4))
stray.strip2 = FrameBuffer(hex_to_rgbw(config["strip2"]["color"]), config["strip2"]["pixels"], NeoPixel(config["strip2"]["pin"], config["strip2"]["pixels"], 4))
stray.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

class LampIdle(AnimatedBehaviour):
    async def draw(self):
        self.lamp.shade.buffer = self.lamp.shade.default_pixels.copy()
        self.lamp.strip1.buffer = self.lamp.strip1.default_pixels.copy()
        self.lamp.strip2.buffer = self.lamp.strip2.default_pixels.copy()

        await self.next_frame()

class LampFadeIn(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.immediate_control = True

    async def draw(self):
        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = fade((0, 0, 0, 0), self.lamp.shade.default_pixels[i], self.frames, self.frame)
        for i in range(self.lamp.strip1.num_pixels):
            self.lamp.strip1.buffer[i] = fade((0, 0, 0, 0), self.lamp.strip1.default_pixels[i], self.frames, self.frame)
        for i in range(self.lamp.strip2.num_pixels):
            self.lamp.strip2.buffer[i] = fade((0, 0, 0, 0), self.lamp.strip2.default_pixels[i], self.frames, self.frame)
        await self.next_frame()

    async def control(self):
        self.play()
        self.stop()

        while True:
            #component will be in the stopped state twice: once on init and once above
            if self.animation_state == AnimationState.STOPPED and self.current_loop > 0:
                self.lamp.base.buffer = self.lamp.base.default_pixels.copy()
                self.lamp.base.previous_buffer = self.lamp.base.default_pixels.copy()

                for behaviour in self.chained_behaviors:
                    self.lamp.behaviour(behaviour).play()
                break

            await asyncio.sleep(0)

stray.add_behaviour(LampFadeIn(stray, frames=30, chained_behaviors=[LampIdle]))
stray.add_behaviour(LampIdle(stray, frames=1))
stray.add_behaviour(SocialGreeting(stray, frames=300))
stray.wake()
