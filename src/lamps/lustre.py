# This lamp has downward facing LEDs and crystals
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
    "base": { "pixels": 40, "color":"#ff7a00", "pin": 14 },
    "lamp": { "name": "lustre", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-lustre" },
    "dmx": { "channel": 4 }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
configurable.base.num_pixels = 24
configurable.base.default_pixels = [
    (180, 50, 150, 3), (22, 0, 150, 40), (255, 0, 29, 0), (0, 78, 40, 255), (94, 160, 180, 2), (180, 180, 180, 4),
    (90, 110, 4, 0), (0, 150, 170, 4), (180, 180, 180, 0), (180, 78, 23, 255), (49, 11, 255, 0), (150, 49, 60, 80),
    (148, 115, 213, 0), (180, 70, 78, 5), (240, 16, 80, 55), (56, 65, 10, 115), (255, 60, 60, 78), (255, 0, 0, 117),
    (0, 250, 250, 87), (255, 14, 80, 0), (115, 115, 115, 115), (240, 240, 80, 0), (156, 160, 29, 45), (70, 70, 180, 180),
]

configurable.add_behaviour(LampDmx(configurable, frames=30, auto_play=True))
configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
