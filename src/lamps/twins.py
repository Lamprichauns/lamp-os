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
    "shade": { "pixels": 40, "color":"#FF00BB", "pin": 12 },
    "base": { "pixels": 40, "color":"#FF0033", "pin": 14 },
    "lamp": { "name": "violo", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-violo" }
})

def post_process(ko_pixels):
    for l in range(12,30):
        ko_pixels[l] = darken(ko_pixels[l], percentage=100)
    for l in range(8,11):
        ko_pixels[l] = brighten(ko_pixels[l], percentage=100)
    for l in range(1,8):
        ko_pixels[l] = darken(ko_pixels[l], percentage=70)

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
twins = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config, post_process_function=post_process)
twins.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
#twins.base.default_pixels = create_gradient((0, 15, 135, 0), (230, 126, 0, 30), steps=config["base"]["pixels"])
#twins.shade.default_pixels = [(0,0,0,140)] * twins.shade.num_pixels

twins.add_behaviour(SocialGreeting(twins, frames=300))
twins.add_behaviour(LampBrightness(twins, frames=1, brightness=twins.brightness))
twins.add_behaviour(Configurator(twins, config=config))
twins.wake()
