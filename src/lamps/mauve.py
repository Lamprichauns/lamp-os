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
    "base": { "pixels": 40, "color":"#ff7a00", "pin": 14 },
    "lamp": { "name": "pine", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-mauve" },
    "dmx": { "channel": 4 }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
configurable.base.default_pixels = create_gradient((255, 0, 70, 50), (120, 0, 90, 35), steps=config["base"]["pixels"])
configurable.shade.default_pixels = [(175, 0, 80, 50)] * configurable.shade.num_pixels

configurable.add_behaviour(LampDmx(configurable, frames=30, auto_play=True))
configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
