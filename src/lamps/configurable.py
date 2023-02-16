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


# c21563
# Define what we'll be setting in the web app
config = {
    "shade": { "pixels": 40, "color":"#ffffff", "pin": 12},
    "base": { "pixels": 40, "color":"ff3423f", "pin": 14},
    "lamp": { "name": "configurable" },
    "wifi": { "ssid": "lamp-eli" }
}

# merge data from the database into the current config
# db is initially an empty json object to initialize the flash
with open("/lamps/files/configurable/db", "r", encoding="utf8") as settings:
    merge_configs(config, load(settings))

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

configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampFadeOut(configurable, frames=100))
configurable.add_behaviour(WebListener(configurable))
configurable.wake()
