# An example of a lamp that can be setup using a web app
from random import choice
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.fade import fade
from utils.gradient import create_gradient
from behaviours.configurator import Configurator, configurator_load_data
from behaviours.lamp_brightness import LampBrightness
from behaviours.lamp_fade_in import LampFadeIn
from behaviours.lamp_idle import LampIdle

#background color wave
shade_colors = [
    { "start": (5, 100, 15, 0), "end": (70, 50, 2, 0) },
    { "start": (50, 0, 43, 0), "end": (120, 25, 15, 0) },
    { "start": (120, 16, 0, 0), "end": (98, 0, 12, 0) },
    { "start": (9, 9, 100, 0), "end": (0, 80, 50, 0) },
    { "start": (20, 100, 1, 0), "end": (150, 45, 0, 0) },
    { "start": (140, 30, 20, 0), "end": (125, 10, 40, 0) },
    { "start": (30, 4, 120, 0), "end": (125, 20, 0, 0) },
    { "start": (9, 125, 4, 0), "end": (0, 60, 120, 0) },
    { "start": (4, 50, 110, 0), "end": (0, 38, 120, 0) },
    { "start": (140, 4, 40, 0), "end": (120, 0, 50, 0) },
    { "start": (133, 24, 0, 0), "end": (150, 50, 0, 0) },
    { "start": (0, 80, 120, 0), "end": (0, 73, 25, 0) }
]

#big spots
spot_colors = [
    { "start": (34, 16, 43, 0), "end": (97, 45, 44, 0) },
    { "start": (120, 16, 0, 0), "end": (98, 0, 12, 0) },
    { "start": (31, 91, 45, 0), "end": (47, 52, 12, 0) },
    { "start": (155, 0, 0, 0), "end": (122, 94, 0, 0) },
    { "start": (155, 44, 0, 0), "end": (122, 119, 0, 0) },
    { "start": (64, 122, 0, 0), "end": (115, 89, 0, 0) },
    { "start": (0, 122, 106, 0), "end": (76, 115, 0, 0) },
    { "start": (0, 88, 122, 0), "end": (115, 114, 0, 0) },
    { "start": (115, 69, 0, 0), "end": (0, 52, 122, 0) },
    { "start": (0, 107, 115, 0), "end": (24, 0, 122, 0) },
    { "start": (0, 54, 115, 0), "end": (122, 0, 78, 0) },
    { "start": (0, 56, 115, 0), "end": (122, 0, 37, 0) },
]

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 40, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 40, "color":"#300783", "pin": 14 },
    "lamp": { "name": "shroom", "default_behaviours": False, "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-shroom" }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.shade.default_pixels = create_gradient(shade_colors[0]["start"], shade_colors[0]["end"], configurable.shade.num_pixels)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

class BackgroundColorFade(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.color_stages = shade_colors
        self.scene_change = False
        self.current_scene = 0
        self.scene_count = len(self.color_stages)
        self.previous_scene_pixels = self.create_scene(self.current_scene)
        self.current_scene_pixels = self.create_scene(self.current_scene)

    def create_scene(self, scene):
        return create_gradient(self.color_stages[scene]["start"], self.color_stages[scene]["end"], self.lamp.shade.num_pixels)

    async def draw(self):
        if self.scene_change is True:
            for i in range(self.lamp.shade.num_pixels):
                self.lamp.shade.buffer[i] = fade(self.previous_scene_pixels[i], self.current_scene_pixels[i], self.frames, self.frame)

            if self.is_last_frame():
                self.scene_change = False
        else:
            self.lamp.shade.buffer = self.current_scene_pixels.copy()

        await self.next_frame()

    async def control(self):
        scene = 0
        while True:
            if self.scene_change:
                await asyncio.sleep(0)
                continue

            scene = choice(range(0, self.scene_count))

            if scene != self.current_scene:
                self.previous_scene_pixels = self.create_scene(self.current_scene)
                self.current_scene_pixels = self.create_scene(scene)

                self.current_scene = scene
                self.frame = 0
                self.scene_change = True

            await asyncio.sleep(1)

class PaintSpots(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.color_stages = spot_colors
        self.scene_change = False
        self.current_scene = -1
        self.spot_count = 9
        self.scene_count = len(self.color_stages)
        self.previous_scene_pixels = create_gradient(shade_colors[0]["start"], shade_colors[0]["end"], self.lamp.shade.num_pixels)[24:24+self.spot_count]
        self.current_scene_pixels = create_gradient(shade_colors[0]["start"], shade_colors[0]["end"], self.lamp.shade.num_pixels)[24:24+self.spot_count]

    def create_scene(self, scene):
        return create_gradient(self.color_stages[scene]["start"], self.color_stages[scene]["end"], self.spot_count)

    async def draw(self):
        if self.scene_change is True:
            for j, i in enumerate(range(24, 33)):
                self.lamp.shade.buffer[i] = fade(self.previous_scene_pixels[j], self.current_scene_pixels[j], self.frames, self.frame)

            if self.is_last_frame():
                self.scene_change = False
        else:
            for j, i in enumerate(range(24, 33)):
                self.lamp.shade.buffer[i] = self.current_scene_pixels[j]

        await self.next_frame()

    async def control(self):
        scene = -1
        while True:
            if self.scene_change:
                await asyncio.sleep(0)
                continue

            scene = choice(range(0, self.scene_count))

            if scene != self.current_scene:
                if self.current_scene == -1:
                    self.current_scene_pixels = self.create_scene(scene)
                else:
                    self.previous_scene_pixels = self.create_scene(self.current_scene)
                    self.current_scene_pixels = self.create_scene(scene)

                self.current_scene = scene
                self.frame = 0
                self.scene_change = True

            await asyncio.sleep(1)

class GlitchedSocialGreeting(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arrived = None
        self.frames = self.frames or 1200
        self.glitch_frames = 23
        self.ease_frames = 60
        self.fade_in_frames = self.glitch_frames + self.ease_frames
        self.use_in_home_mode = False
        self.glitch_colors = create_gradient((0,45,200, 0), (180,0,60,0), steps=self.lamp.shade.num_pixels)

    def glitch(self):
        offset = choice(range(0, self.lamp.shade.num_pixels))
        return self.glitch_colors[offset:] + self.glitch_colors[:offset]

    async def draw(self):
        for i in range(self.lamp.shade.num_pixels):
            if self.frame < self.glitch_frames:
                self.lamp.shade.buffer = self.glitch()

            elif self.frame < self.fade_in_frames:
                self.lamp.shade.buffer[i] = fade(self.lamp.shade.buffer[i], self.arrived["base_color"], self.fade_in_frames, self.frame)

            elif self.frame > self.frames-self.ease_frames:
                self.lamp.shade.buffer[i] = fade(self.arrived["base_color"], self.lamp.shade.buffer[i], self.ease_frames, self.frame % self.ease_frames)

            else:
                self.lamp.shade.buffer[i] = self.arrived["base_color"]

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

configurable.add_behaviour(LampFadeIn(configurable, frames=30, chained_behaviors=[LampIdle, BackgroundColorFade, PaintSpots]))
configurable.add_behaviour(LampIdle(configurable, frames=1))
configurable.add_behaviour(BackgroundColorFade(configurable, frames=2400))
configurable.add_behaviour(PaintSpots(configurable, frames=1200))
configurable.add_behaviour(GlitchedSocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
