# An example of a lamp that can be setup using a web app
from components.network.access_point import AccessPoint
from lamp_core.standard_lamp import StandardLamp
from utils.color_tools import darken
from behaviours.configurator import Configurator, configurator_load_data
from behaviours.social import SocialGreeting
from behaviours.lamp_brightness import LampBrightness
from behaviours.lamp_dmx import LampDmx

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 36, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 40, "color":"#300783", "pin": 14 },
    "lamp": { "name": "configurable", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-configurable" },
    "dmx": { "channel": 4 }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

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

configurable.add_behaviour(LampDmx(configurable, frames=30, auto_play=True))
configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
