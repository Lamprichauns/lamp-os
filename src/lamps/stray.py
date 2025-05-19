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

def generate_color():
    valid = False
    count = 0
    while not valid and count < 200:
        count += 1
        r = round(random.random() * 75) + 15
        g = round(random.random() * 75) + 15
        b = round(random.random() * 75) + 15
        sum_rgb = r + g + b
        if 45 < sum_rgb < 270:
            vibrance = (
                (max(r, g) - min(r, g)) +
                (max(g, b) - min(g, b)) +
                (max(b, r) - min(b, r))
            ) / (255 * 3)
            if vibrance > 0.15:
                return (r, g, b, 0)
    return (20, 20, 20, 0)  # default color

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 0, "color":"#000000", "pin": 25 },
    "strip_large_spots": { "pixels": 9, "color":"#300783", "pin": 26 },
    "strip_medium_spots": { "pixels": 12, "color":"#300783", "pin": 27 },
    "strip_small_spots": { "pixels": 20, "color":"#300783", "pin": 14 },
    "base": { "pixels": 50, "color":"#300783", "pin": 12 },
    "lamp": { "name": "shroom", "default_behaviours": False, "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-shroom" }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])
stray = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["strip_large_spots"]["color"], config_opts=config)
stray.strip_large_spots = FrameBuffer(hex_to_rgbw(config["strip_large_spots"]["color"]), config["strip_large_spots"]["pixels"], NeoPixel(config["strip_large_spots"]["pin"], config["strip_large_spots"]["pixels"], 4))
stray.strip_medium_spots = FrameBuffer(hex_to_rgbw(config["strip_medium_spots"]["color"]), config["strip_medium_spots"]["pixels"], NeoPixel(config["strip_medium_spots"]["pin"], config["strip_medium_spots"]["pixels"], 4))
stray.strip_small_spots = FrameBuffer(hex_to_rgbw(config["strip_small_spots"]["color"]), config["strip_small_spots"]["pixels"], NeoPixel(config["strip_small_spots"]["pin"], config["strip_small_spots"]["pixels"], 3, ColorOrder.RGBW))
stray.strip_large_spots.default_pixels = create_gradient(generate_color(), generate_color(), config["strip_large_spots"]["pixels"])
stray.strip_medium_spots.default_pixels = create_gradient(generate_color(), generate_color(), config["strip_medium_spots"]["pixels"])
stray.strip_small_spots.default_pixels = create_gradient(generate_color(), generate_color(), config["strip_small_spots"]["pixels"])
stray.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

class PaintSpots(AnimatedBehaviour):
    def __init__(self, strip=None, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.auto_play = True
        self.strip = strip
        self.scene_change = False
        self.spot_count = self.strip.num_pixels
        self.previous_scene_pixels = create_gradient(generate_color(), generate_color(), self.spot_count)
        self.current_scene_pixels = create_gradient(generate_color(), generate_color(), self.spot_count)

    def create_scene(self, scene):
        return create_gradient(generate_color(), generate_color(), self.spot_count)

    async def draw(self):
        if self.scene_change is True:
            for j, i in enumerate(range(0, self.spot_count)):
                self.strip.buffer[i] = fade(self.previous_scene_pixels[j], self.current_scene_pixels[j], self.frames, self.frame)

            if self.is_last_frame():
                self.scene_change = False
        else:
            for j, i in enumerate(range(0,  self.spot_count)):
                self.strip.buffer[i] = self.current_scene_pixels[j]

        await self.next_frame()

    async def control(self):
        scene = -1
        while True:
            if self.scene_change:
                await asyncio.sleep(0)
                continue

            self.previous_scene_pixels = self.current_scene_pixels
            self.current_scene_pixels = self.create_scene(scene)

            self.frame = 0
            self.scene_change = True

            await asyncio.sleep(1)

class PaintSpotsMedium(PaintSpots):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

class PaintSpotsSmall(PaintSpots):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

class GlitchedSocialGreeting(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arrived = None
        self.frames = self.frames or 1200
        self.glitch_frames = 23
        self.ease_frames = 60
        self.fade_in_frames = self.glitch_frames + self.ease_frames
        self.use_in_home_mode = False
        self.glitch_colors = create_gradient((0,45,200, 0), (180,0,60,0), steps=self.lamp.strip_small_spots.num_pixels)

    def glitch(self, buffer):
        offset = choice(range(0, buffer.num_pixels))
        return self.glitch_colors[offset:] + self.glitch_colors[:offset]

    async def draw(self):
        for i in range(self.lamp.strip_large_spots.num_pixels):
            if self.frame < self.glitch_frames:
                self.lamp.strip_large_spots.buffer = self.glitch(self.lamp.strip_large_spots)

            elif self.frame < self.fade_in_frames:
                self.lamp.strip_large_spots.buffer[i] = fade(self.lamp.strip_large_spots.buffer[i], self.arrived["base_color"], self.fade_in_frames, self.frame)

            elif self.frame > self.frames-self.ease_frames:
                self.lamp.strip_large_spots.buffer[i] = fade(self.arrived["base_color"], self.lamp.strip_large_spots.buffer[i], self.ease_frames, self.frame % self.ease_frames)

            else:
                self.lamp.strip_large_spots.buffer[i] = self.arrived["base_color"]


        for i in range(self.lamp.strip_medium_spots.num_pixels):
            if self.frame < self.glitch_frames:
                self.lamp.strip_medium_spots.buffer = self.glitch(self.lamp.strip_medium_spots)

            elif self.frame < self.fade_in_frames:
                self.lamp.strip_medium_spots.buffer[i] = fade(self.lamp.strip_medium_spots.buffer[i], self.arrived["base_color"], self.fade_in_frames, self.frame)

            elif self.frame > self.frames-self.ease_frames:
                self.lamp.strip_medium_spots.buffer[i] = fade(self.arrived["base_color"], self.lamp.strip_medium_spots.buffer[i], self.ease_frames, self.frame % self.ease_frames)

            else:
                self.lamp.strip_medium_spots.buffer[i] = self.arrived["base_color"]

        for i in range(self.lamp.strip_small_spots.num_pixels):
            if self.frame < self.glitch_frames:
                self.lamp.strip_small_spots.buffer = self.glitch(self.lamp.strip_small_spots)

            elif self.frame < self.fade_in_frames:
                self.lamp.strip_small_spots.buffer[i] = fade(self.lamp.strip_small_spots.buffer[i], self.arrived["base_color"], self.fade_in_frames, self.frame)

            elif self.frame > self.frames-self.ease_frames:
                self.lamp.strip_small_spots.buffer[i] = fade(self.arrived["base_color"], self.lamp.strip_small_spots.buffer[i], self.ease_frames, self.frame % self.ease_frames)
            else:
                self.lamp.strip_small_spots.buffer[i] = self.arrived["base_color"]

        await self.next_frame()

    async def control(self):
        while True:
            # wait for lamp to be started up for a while on first boot
            await asyncio.sleep(20)

            if self.animation_state not in (AnimationState.PLAYING, AnimationState.STOPPING):
                arrived = await self.lamp.network.arrived()
                self.arrived = arrived
                print("%s has arrived" % (arrived["name"]))
                self.play()
                self.stop()

class LampIdle(AnimatedBehaviour):
    async def draw(self):
        self.lamp.strip_large_spots.buffer = self.lamp.strip_large_spots.default_pixels.copy()
        self.lamp.strip_medium_spots.buffer = self.lamp.strip_medium_spots.default_pixels.copy()
        self.lamp.strip_small_spots.buffer = self.lamp.strip_small_spots.default_pixels.copy()
        self.lamp.base.buffer = self.lamp.base.default_pixels.copy()

        await self.next_frame()

class LampFadeIn(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.immediate_control = True

    async def draw(self):
        for i in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[i] = fade((0, 0, 0, 0), self.lamp.base.default_pixels[i], self.frames, self.frame)

        for i in range(self.lamp.strip_large_spots.num_pixels):
            self.lamp.strip_large_spots.buffer[i] = fade((0, 0, 0, 0), self.lamp.strip_large_spots.default_pixels[i], self.frames, self.frame)
        for i in range(self.lamp.strip_medium_spots.num_pixels):
            self.lamp.strip_medium_spots.buffer[i] = fade((0, 0, 0, 0), self.lamp.strip_medium_spots.default_pixels[i], self.frames, self.frame)
        for i in range(self.lamp.strip_small_spots.num_pixels):
            self.lamp.strip_small_spots.buffer[i] = fade((0, 0, 0, 0), self.lamp.strip_small_spots.default_pixels[i], self.frames, self.frame)
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

stray.add_behaviour(LampFadeIn(stray, frames=30, chained_behaviors=[LampIdle, PaintSpots, PaintSpotsMedium, PaintSpotsSmall]))
stray.add_behaviour(LampIdle(stray, frames=1))
stray.add_behaviour(PaintSpots(stray.strip_large_spots, stray, frames=3200))
stray.add_behaviour(PaintSpotsMedium(stray.strip_medium_spots, stray, frames=4500))
stray.add_behaviour(PaintSpotsSmall(stray.strip_small_spots, stray, frames=2500))
stray.add_behaviour(GlitchedSocialGreeting(stray, frames=300))
stray.wake()
