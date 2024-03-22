# An example of a lamp that can be setup using a web app
from components.network.access_point import AccessPoint
from lamp_core.standard_lamp import StandardLamp
from utils.color_tools import darken, brighten
from utils.gradient import create_gradient
from behaviours.configurator import Configurator, configurator_load_data
from behaviours.social import SocialGreeting
from behaviours.lamp_brightness import LampBrightness

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 40, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 40, "color":"#c87e00", "pin": 14 },
    "lamp": { "name": "nite", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-hash" }
})

def post_process(ko_pixels):
    for l in range(25,30):
        ko_pixels[l] = darken(ko_pixels[l], percentage=100)
    for l in range(10,20):
        ko_pixels[l] = brighten(ko_pixels[l], percentage=50)
    for l in range(1,8):
        ko_pixels[l] = darken(ko_pixels[l], percentage=70)

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config, post_process_function=post_process)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
configurable.base.default_pixels = create_gradient((0, 15, 135, 0), (230, 126, 0, 30), steps=config["base"]["pixels"])
configurable.shade.default_pixels = [(0,0,0,140)] * configurable.shade.num_pixels

configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
