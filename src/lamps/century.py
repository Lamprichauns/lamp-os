# Century lamp
from time import sleep
import uasyncio as asyncio
from behaviours.lamp_fade_in import LampFadeIn
from lamp_core.behaviour import BackgroundBehavior, AnimatedBehaviour
from lamp_core.standard_lamp import StandardLamp
from components.motion.motion_6050 import MotionMPU6050
from components.network.access_point import AccessPoint
from components.temperature.temperature_6050 import TemperatureMPU6050
from utils.color_tools import brighten, darken
from utils.gradient import create_gradient
from utils.fade import fade
from utils.temperature import get_temperature_index
from vendor import tinyweb

sleep(1)

# for ease of use, you can define a config to flow into all the components
config = {
    "shade": { "pin": 12, "pixels": 40, "bpp": 3},
    "base": { "pin": 13, "pixels": 60, "bpp": 3},
    "fade_in": False,
    "motion": { "pin_sda": 21, "pin_scl": 22},
    "sunset": {"temperature_low": 30, "temperature_high": 40 },
}

# knockout and brighten some pixels for all scenes
def post_process(pixels):
    for j in range(32,37):
        pixels[j] = darken(pixels[j], percentage=85)
    for j in range(20,25):
        pixels[j] = brighten(pixels[j], percentage=15)

# Compose all the components
century = StandardLamp("century", "#270000", "#931702", config, post_process_function = post_process)
century.motion = MotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])
century.temperature = TemperatureMPU6050(century.motion.accelerometer)
century.access_point = AccessPoint(ssid="century-lamp", password="123456789")

# Setup first sunset scene
century.base.default_pixels = create_gradient((158, 10, 3, 0), (220, 80, 8, 0), steps=config["base"]["pixels"])

# Kick off the web server for lamp remote controls
app = tinyweb.webserver()

# Handle new values
class Configurator():
    def post(self, data):
        #set as an override
        config["sunset"]["temperature_low"] = int(data["temperature_low"])
        config["sunset"]["temperature_high"] = int(data["temperature_high"])

        # write the values to flash
        new_settings = {
            "temperature_low": int(data["temperature_low"]),
            "temperature_high": int(data["temperature_high"])
        }

        print(data)
        return {'message': 'OK'}, 200

# Index page
@app.route('/')
async def index(_, response):
    await response.start_html()
    with open("/lamps/files/century/configurator.html", mode="r", encoding="utf8") as file:
        html = file.read()

    await response.send(html)

app.add_resource(Configurator, '/settings')

# Start listening for connections on port 80
class WebListener(BackgroundBehavior):
    async def run(self):
        app.run(host='0.0.0.0', port=80)

# This behavior will create a very slow cooling/warming color change in reaction the ambient temperature
class Sunset(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.sunset_stages = [
            { "start": (5, 33, 90, 0), "end": (30, 90, 12, 0) },
            { "start": (16, 7, 142, 0), "end": (85, 115, 16, 0) },
            { "start": (52, 4, 107, 0), "end": (104, 180, 15, 0) },
            { "start": (107, 5, 57, 0), "end": (155, 140, 30, 0) },
            { "start": (108, 4, 23, 0), "end": (181, 96, 14, 0) },
            { "start": (108, 13, 3, 0), "end": (217, 68, 30, 0) },
            { "start": (150, 10, 0, 0), "end": (220, 80, 8, 0) },
        ]
        self.scene_change = False
        self.current_scene = 6
        self.previous_scene_pixels = self.create_scene(self.current_scene)
        self.current_scene_pixels = self.create_scene(self.current_scene)

    def create_scene(self, scene):
        return create_gradient(self.sunset_stages[scene]["start"], self.sunset_stages[scene]["end"], config["base"]["pixels"])

    async def draw(self):
        if self.scene_change is True:
            for i in range(config["base"]["pixels"]):
                self.lamp.base.buffer[i] = fade(self.previous_scene_pixels[i], self.current_scene_pixels[i], self.frame, self.frames)

            if self.frame == self.frames-1:
                self.scene_change = False
        else:
            self.lamp.base.buffer = self.current_scene_pixels

        await self.next_frame()

    async def control(self):
        scene = self.current_scene

        while True:
            if self.scene_change:
                await asyncio.sleep(0)
                continue

            #scene = self.get_scene_for_temperature()
            scene += 1
            if scene > 6:
                scene = 0

            if scene != self.current_scene:
                print("Scene change {}".format(scene))

                # precompute the scenes
                self.previous_scene_pixels = self.create_scene(self.current_scene)
                self.current_scene_pixels = self.create_scene(scene)

                self.current_scene = scene
                self.frame = 0
                self.scene_change = True

            await asyncio.sleep(10)

century.add_behaviour(LampFadeIn(century, frames=30, chained_behaviors=[Sunset]))
century.add_behaviour(Sunset(century, frames=600))
century.add_behaviour(WebListener(century))

century.wake()
