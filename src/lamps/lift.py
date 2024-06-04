# Lift is a lamp that blows bubbles on social interaction with a built in bubble
# gun mounted to the side of the bulb
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
from lamp_core.behaviour import AnimatedBehaviour

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 40, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 40, "color":"#249147", "pin": 14 },
    "lamp": { "name": "lift", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-lift" },
    "dmx": { "channel": 4 }
})

def post_process(ko_pixels):
    for l in range(23,40):
        ko_pixels[l] = darken(ko_pixels[l], percentage=90)

    ko_pixels[19]  = (0,0,0,0)

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config, post_process_function=post_process)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
configurable.base.default_pixels = create_gradient((14, 100, 48, 0), (36, 145, 71, 21), steps=config["base"]["pixels"])
configurable.shade.default_pixels = [(136,61,0,180)] * configurable.shade.num_pixels

class LampBubbles(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.motor_pin = PWM(Pin(27))
        self.last_dmx_message = None
        self.lamp.dmx.add_update_callback(self.dmx_update)
        self.lamp.dmx.add_timeout_callback(self.dmx_timeout)

    def dmx_update(self, message):
        self.last_dmx_message = message
        self.motor_pin.duty(self.last_dmx_message[9]*4)

    def dmx_timeout(self):
        self.motor_pin.duty(0)

    async def draw(self):
        await self.next_frame()

    async def control(self):
        while True:

            await asyncio.sleep(0)

configurable.add_behaviour(LampDmx(configurable, frames=30, auto_play=True))
configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBubbles(configurable, frames=1, auto_play=True))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
