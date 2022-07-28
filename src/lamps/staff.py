# An example of a lamp that can be setup using a web app
import re
from ujson import load, dump
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.behaviour import BackgroundBehavior
from lamp_core.standard_lamp import StandardLamp
from utils.config import merge_configs
from utils.color_tools import darken
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_out import LampFadeOut
from vendor import tinyweb
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from lamp_core.standard_lamp import StandardLamp
from behaviours.lamp_fade_in import LampFadeIn
from utils.gradient import create_gradient
from utils.fade import pingpong_fade, fade
from machine import Pin
from components.network.bluetooth import Bluetooth

# Define what we'll be setting in the web app
config = {
    "shade": { "pixels": 40, "color":"#FF0000", "pin": 12 },
    "base": { "pixels": 40, "color":"#FF0000", "pin": 14 },
    "lamp": { "name": "staff", "default_behaviours": False },
    "wifi": { "ssid": "lamp-290319" }
}
# 27 is the button

# merge data from the database into the current config
# db is initially an empty json object to initialize the flash
with open("/lamps/files/configurable/db", "r", encoding="utf8") as settings:
    merge_configs(config, load(settings))

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

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

class Rainbow(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        red = (255,0,0,0)
        orange = (255, 20, 0, 0)
        yellow = (255, 60, 0, 0)
        green = (0, 255, 0, 0)
        blue = (0, 0, 255, 0)
        indigo = (60, 0, 255, 0)
        violet = (255, 0, 255, 0)
        self.colors = [red, orange, yellow, green, blue, indigo, violet]
        self.previous_color = 0
        self.current_color = 0
        self.is_paused = False

    async def draw(self):
        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = fade(self.colors[self.previous_color], self.colors[self.current_color], self.frames, self.frame)
            self.lamp.base.buffer[i] = fade(self.colors[self.previous_color], self.colors[self.current_color], self.frames, self.frame)

        await self.next_frame()

    async def control(self):
        push_button = Pin(4, Pin.IN, Pin.PULL_DOWN)

        while True:
            button_state = push_button.value()

            if button_state == 0:
                print("Pushed %s" % (button_state))

                #if AnimationState.PAUSED:
                #    print("Unpause")
                #    self.is_paused = False
                #    self.play()
                #else:
                color_rgb = self.lamp.shade.buffer[0]
                color_rgb = color_rgb[:-1]

                color = self.lamp.bluetooth._rgb_to_hex(color_rgb)

                print("Color set to %s" % (color))

                self.lamp.bluetooth.ble.active(False)
                await asyncio.sleep(5)
                self.lamp.bluetooth = Bluetooth(self.lamp.name, color, color)
                self.lamp.network = self.lamp.bluetooth.network
                self.lamp.bluetooth.enable()


            if self.animation_state in (AnimationState.STOPPED, AnimationState.PAUSED):
                self.previous_color = self.current_color
                self.current_color += 1

                if self.current_color > 6:
                    self.current_color = 0

                self.play()
                self.stop()

            await asyncio.sleep_ms(50)

configurable.add_behaviour(LampFadeIn(configurable, frames=30))
configurable.add_behaviour(Rainbow(configurable, frames=500))
configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampFadeOut(configurable, frames=30))
configurable.add_behaviour(WebListener(configurable))
configurable.wake()
