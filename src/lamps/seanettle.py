# Sea Nettle
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
    "wifi": { "ssid": "lamp-seanettle" },
    "dmx": { "channel": 4 }
})

def post_process(ko_pixels):
    for l in range(23,40):
        ko_pixels[l] = darken(ko_pixels[l], percentage=90)

    ko_pixels[19]  = (0,0,0,0)

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
seanettle = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config, post_process_function=post_process)
seanettle.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
seanettle.shade.default_pixels = [(196,81,33,100)] * seanettle.shade.num_pixels
seanettle.base.default_pixels = [(175,40,50,100)] * seanettle.base.num_pixels

seanettle.add_behaviour(LampDmx(seanettle, frames=30, auto_play=True))
seanettle.add_behaviour(SocialGreeting(seanettle, frames=300))
seanettle.add_behaviour(LampBrightness(seanettle, frames=1, brightness=seanettle.brightness))
seanettle.add_behaviour(Configurator(seanettle, config=config))
seanettle.wake()
