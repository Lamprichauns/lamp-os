# An example of a lamp that can be setup using a web app
import re
from random import choice, randrange
from ujson import load, dump
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.behaviour import BackgroundBehavior, AnimatedBehaviour
from lamp_core.standard_lamp import StandardLamp
from utils.config import merge_configs
from utils.color_tools import darken, brighten
from utils.easing import pingpong_ease
from utils.fade import pingpong_fade
from utils.gradient import create_gradient
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_out import LampFadeOut
from behaviours.lamp_brightness import LampBrightness
from vendor import tinyweb

# Define what we'll be setting in the web app
config = {
    "shade": { "pixels": 40, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 40, "color":"#300783", "pin": 14 },
    "lamp": { "name": "nite", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-nite" }
}

def post_process(ko_pixels):
    for l in range(18,29):
        ko_pixels[l] = darken(ko_pixels[l], percentage=80)

# merge data from the database into the current config
# db is initially an empty json object to initialize the flash
with open("/lamps/files/configurable/db", "r", encoding="utf8") as settings:
    merge_configs(config, load(settings))

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config, post_process_function=post_process)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
configurable.base.default_pixels = create_gradient((108, 4, 23, 0), (181, 60, 14, 0), steps=config["base"]["pixels"])
configurable.shade.default_pixels = [(0,0,0,140)] * configurable.shade.num_pixels


# To handle pastel colors and cool whites, we can darken the rgb values for a normal 40x40 lamp to not draw too much current
# tested with shade: #D486FE (pastel purple), base: #FF9494 (pastel rose) to get a 5V voltage of 4.8V
# This will ensure lamps can stay electrically stable for many hours even on extreme color settings
# This routine won't modify warm white values
for i in range(config["base"]["pixels"]):
    if configurable.base.default_pixels[i][3] == 255:
        break

    configurable.base.default_pixels[i] = darken(configurable.base.default_pixels[i], 30)

for i in range(config["shade"]["pixels"]):
    if configurable.shade.default_pixels[i][3] == 255:
        break

    configurable.shade.default_pixels[i] = darken(configurable.shade.default_pixels[i], 30)

# Web service init
app = tinyweb.webserver()

# Read and amend the config object
class Configurator():
    def get(self, _):
        return config

    def post(self, data):
        print(data)
        name_sanitizer = re.compile('[^a-z]')
        number_sanitizer = re.compile('[^0-9]+')
        config["shade"]["color"] = data["shade"]
        config["base"]["color"] = data["base"]
        config["shade"]["pixels"] = abs(int(number_sanitizer.sub("", data["shade_pixels"])))
        config["base"]["pixels"] = abs(int(number_sanitizer.sub("", data["base_pixels"])))
        config["lamp"]["name"] = name_sanitizer.sub("", data["name"])
        config["lamp"]["brightness"] = abs(int(number_sanitizer.sub("", data["brightness"])))
        config["lamp"]["home_mode"] = data.get("home_mode") == "on"

        if not config["lamp"]["name"]:
            return {'message': 'bad name'}, 500

        try:
            with open("/lamps/files/configurable/db", "w", encoding="utf8") as flash:
                dump(config, flash)
        except Exception as e:
            print(e)

        configurable.behaviour(LampFadeOut).play()

        return {'message': 'OK'}, 200

@app.route('/')
async def index(_, resp):
    await resp.send_file("/lamps/files/configurable/configurator.html")

app.add_resource(Configurator, '/settings')

# Start listening for connections on port 80
class WebListener(BackgroundBehavior):
    async def run(self):
        await asyncio.sleep(5)
        app.run(host='0.0.0.0', port=80)

class Sun(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.sun_position = 12
        self.auto_play=True

    async def draw(self):
        self.lamp.base.buffer[self.sun_position] = pingpong_fade(self.lamp.base.buffer[self.sun_position], (255, 180, 40, 120), self.lamp.base.buffer[self.sun_position], self.frames, self.frame)
        await self.next_frame()

    async def control(self):
        while True:
            if self.frame == 0:
                self.sun_position = choice(range(5, 18))
            await asyncio.sleep(0)

class EveningSky(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.cloud_positions = []
        self.cloud_brightness = 10
        self.cloud_color = (240, 17, 220, 0)
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
                self.cloud_positions = [randrange(18, 30, 1) for i in range(1)]
                self.cloud_style = choice(range(0, 1))
            await asyncio.sleep(0)

configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(Sun(configurable, frames=1860, auto_play=True))
configurable.add_behaviour(EveningSky(configurable, frames=180, auto_play=True))
configurable.add_behaviour(LampFadeOut(configurable, frames=100))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(WebListener(configurable))
configurable.wake()
