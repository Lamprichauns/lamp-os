# Century lamp
from random import randrange, choice
import re
from ujson import dump, load
import uasyncio as asyncio
from behaviours.lamp_fade_in import LampFadeIn
from behaviours.lamp_fade_out import LampFadeOut
from behaviours.social import SocialGreeting
from lamp_core.behaviour import AnimatedBehaviour, AnimationState, BackgroundBehavior
from lamp_core.standard_lamp import StandardLamp
from components.motion.motion_6050 import MotionMPU6050
from components.network.access_point import AccessPoint
from components.temperature.temperature_6050 import TemperatureMPU6050
from utils.color_tools import brighten, darken
from utils.gradient import create_gradient
from utils.fade import fade, pingpong_fade
from utils.temperature import get_temperature_index
from utils.easing import pingpong_ease
from utils.config import merge_configs
from vendor import tinyweb

# for ease of use, you can define a config to flow into all the components
config = {
    "shade": { "pin": 13, "pixels": 40, "bpp": 4, "color":"#FFFFFF"},
    "base": { "pin": 12, "pixels": 50, "bpp": 4,  "color":"#931702"},
    "lamp": { "default_behaviours": False, "debug": True, "name": "century" },
    "motion": { "pin_sda": 21, "pin_scl": 22, "threshold": 3000 },
    "sunset": {"low": 30, "high": 40, "current": 0 },
}

with open("/lamps/files/century/db", "r", encoding="utf8") as settings:
    merge_configs(config, load(settings))

# knockout and brighten some pixels for all scenes
# add some gamma correction to save power
def post_process(ko_pixels):
    for l in range(30,36):
        ko_pixels[l] = darken(ko_pixels[l], percentage=85)
    for l in range(0, 40):
        ko_pixels[l] = darken(ko_pixels[l], percentage=20)

# Compose all the components
century = StandardLamp(config["lamp"]["name"], config["base"]["color"], config["shade"]["color"], config, post_process_function = post_process)
century.motion = MotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])
century.temperature = TemperatureMPU6050(century.motion.accelerometer)
century.access_point = AccessPoint(ssid="century-lamp", password="123456789")
century.base.default_pixels = create_gradient((150, 10, 0, 0), (220, 100, 8, 0), steps=config["base"]["pixels"])
for j in range(20,25):
    pixels = brighten(century.base.default_pixels[j], percentage=200)
    century.base.default_pixels[j] = (pixels[0], pixels[1], pixels[2], 40)
century.shade.default_pixels = [(0,0,0,180)]*config["shade"]["pixels"]

# Web svc init
app = tinyweb.webserver()

# Read and amend the config object
class Configurator():
    def get(self, _):
        config["sunset"]["current"] = century.temperature.get_temperature_value()
        return config

    def post(self, data):
        print(data)
        name_sanitizer = re.compile('[^a-z]')
        number_sanitizer = re.compile('[^0-9]+')
        config["shade"]["color"] = data["shade"]
        config["base"]["color"] = data["base"]
        config["lamp"]["name"] = name_sanitizer.sub("", data["name"])
        config["motion"]["threshold"] = abs(int(number_sanitizer.sub("", data["threshold"])))
        config["sunset"]["low"] = abs(int(number_sanitizer.sub("", data["low"])))
        config["sunset"]["high"] = abs(int(number_sanitizer.sub("", data["high"])))

        if not config["lamp"]["name"]:
            return {'message': 'bad name'}, 500

        try:
            with open("/lamps/files/century/db", "w", encoding="utf8") as flash:
                dump(config, flash)
        except Exception as e:
            print(e)

        century.behaviour(LampFadeOut).play()

        return {'message': 'OK'}, 200

@app.route('/')
async def index(_, resp):
    await resp.send_file("/lamps/files/century/configurator.html")

app.add_resource(Configurator, '/settings')

# Start listening for connections on port 80
class WebListener(BackgroundBehavior):
    async def run(self):
        await asyncio.sleep(5)
        app.run(host='0.0.0.0', port=80)

# A very slow cooling/warming color change in reaction the ambient temperature
class Sunset(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.sunset_stages = [
            { "start": (5, 33, 90, 0), "end": (30, 30, 12, 20) },
            { "start": (16, 7, 142, 0), "end": (85, 42, 16, 0) },
            { "start": (52, 4, 107, 0), "end": (104, 40, 15, 0) },
            { "start": (107, 5, 57, 0), "end": (155, 50, 30, 0) },
            { "start": (108, 4, 23, 0), "end": (181, 60, 14, 0) },
            { "start": (108, 13, 3, 0), "end": (217, 68, 30, 0) },
            { "start": (150, 10, 0, 0), "end": (220, 100, 8, 0) },
        ]
        self.scene_change = False
        self.current_scene = -1
        self.previous_scene_pixels = self.create_scene(self.current_scene)
        self.current_scene_pixels = self.create_scene(self.current_scene)

    def create_scene(self, scene):
        return create_gradient(self.sunset_stages[scene]["start"], self.sunset_stages[scene]["end"], config["base"]["pixels"])

    async def draw(self):
        if self.scene_change is True:
            for i in range(config["base"]["pixels"]):
                self.lamp.base.buffer[i] = fade(self.previous_scene_pixels[i], self.current_scene_pixels[i], self.frames, self.frame)

            if self.is_last_frame():
                self.scene_change = False
        else:
            self.lamp.base.buffer = self.current_scene_pixels.copy()

        for k in range(20,25):
            glow_pixels = brighten(self.lamp.base.buffer[k], percentage=200)
            self.lamp.base.buffer[k] = (glow_pixels[0], glow_pixels[1], glow_pixels[2], 40)

        await self.next_frame()

    async def control(self):
        while True:
            if self.scene_change:
                await asyncio.sleep(0)
                continue

            scene = get_temperature_index(self.lamp.temperature.get_temperature_value(), config["sunset"]["low"], config["sunset"]["high"], 7)

            if scene != self.current_scene:
                print("Scene change {}: Temperature: {}".format(scene, self.lamp.temperature.get_temperature_value()))

                self.previous_scene_pixels = self.create_scene(self.current_scene)
                self.current_scene_pixels = self.create_scene(scene)

                if scene < 2:
                    self.lamp.behaviour(StarShade).play()
                else:
                    self.lamp.behaviour(StarShade).stop()

                self.current_scene = scene
                self.frame = 0
                self.scene_change = True

            await asyncio.sleep(20)

# Gently vibe to the rhythm if the lamp is dancing
class DanceReaction(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.last_accelerometer_value = 0
        self.polling_interval = 100
        self.dance_gesture_peak = config["motion"]["threshold"]
        self.pixel_list = []

    def within_range(self, value, baseline, threshold):
        return baseline - threshold > value < baseline + threshold

    async def draw(self):
        percentage = pingpong_ease(0, 100, 0, self.frames, self.frame)

        for i in self.pixel_list:
            self.lamp.base.buffer[i] = brighten(self.lamp.base.buffer[i], percentage)

        await self.next_frame()

    async def control(self):
        while True:
            if self.animation_state in(AnimationState.PLAYING, AnimationState.STOPPING):
                await asyncio.sleep_ms(self.polling_interval)
                continue

            value = self.lamp.motion.get_movement_intensity_value()

            if (value >= self.dance_gesture_peak and value is not self.within_range(value, self.last_accelerometer_value, 100)):
                self.last_accelerometer_value = value

                self.pixel_list = [randrange(0, self.lamp.base.num_pixels, 1) for i in range(int(self.lamp.base.num_pixels/2))]

                self.play()
                self.stop()

            await asyncio.sleep_ms(self.polling_interval)

# Add some sunlight
class Sun(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.sun_position = 12

    async def draw(self):
        self.lamp.base.buffer[self.sun_position] = pingpong_fade(self.lamp.base.buffer[self.sun_position], (255, 180, 40, 120), self.lamp.base.buffer[self.sun_position], self.frames, self.frame)
        await self.next_frame()

    async def control(self):
        while True:
            if self.frame == 0:
                self.sun_position = choice(range(12, 20))
            await asyncio.sleep(0)

# Add some point lights to the scenes and move em around
class EveningSky(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.cloud_positions = []
        self.cloud_brightness = 145
        self.cloud_color = (240, 17, 220, 0)
        self.cloud_style = 0

    async def draw(self):

        if self.cloud_style == 0:
            percentage = pingpong_ease(0, self.cloud_brightness, 0, self.frames, self.frame)

            for i in self.cloud_positions:
                self.lamp.base.buffer[i] = brighten(self.lamp.base.buffer[i], percentage)

        else:
            for i in self.cloud_positions:
                self.lamp.base.buffer[i] = pingpong_fade(self.lamp.base.buffer[i], self.cloud_color, self.lamp.base.buffer[i], self.frames, self.frame)

        self.lamp.shade.buffer = self.lamp.shade.default_pixels.copy()

        await self.next_frame()

    async def control(self):
        while True:
            if self.frame == 0:
                self.cloud_brightness = choice(range(145, 185))
                self.cloud_positions = [randrange(22, 30, 1) for i in range(1)]
                self.cloud_style = choice(range(0, 1))
            await asyncio.sleep(0)

# have the shade change and dance with the temperature
class StarShade(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.star_positions = []
        self.star_color = (110, 170, 250, 10)

    async def draw(self):
        for i in self.star_positions:
            self.lamp.shade.buffer[i] = pingpong_fade(self.lamp.shade.buffer[i], self.star_color, self.lamp.shade.buffer[i], self.frames, self.frame)

        await self.next_frame()

    async def control(self):
        while True:
            if self.frame == 0:
                self.star_positions = [randrange(0, 15, 1) for i in range(3,7)]

            await asyncio.sleep(0)

century.add_behaviour(LampFadeIn(century, frames=30, chained_behaviors=[Sunset, Sun, EveningSky]))
century.add_behaviour(Sunset(century, frames=3000))
century.add_behaviour(Sun(century, frames=1860))
century.add_behaviour(EveningSky(century, frames=300))
century.add_behaviour(StarShade(century, frames=220))
century.add_behaviour(SocialGreeting(century, frames=300))
century.add_behaviour(DanceReaction(century, frames=16))
century.add_behaviour(WebListener(century))
century.add_behaviour(LampFadeOut(century, frames=30))

century.wake()
