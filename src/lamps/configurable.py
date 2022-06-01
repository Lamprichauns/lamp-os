# An example of a lamp that can be setup using a web app
from ujson import load, dump
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.behaviour import BackgroundBehavior
from lamp_core.standard_lamp import StandardLamp
from utils.config import merge_configs
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_out import LampFadeOut
from vendor import tinyweb
import re

# Define what we'll be setting in the web app
config = {
    "shade": { "pixels": 40, "color":"#220000"},
    "base": { "pixels": 40, "color":"#002200"},
    "lamp": { "name": "configurable", "debug": True },
    "wifi": { "ssid": "lamp-290309" }
}

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

configurable.add_behaviour(SocialGreeting(configurable, frames=300))
configurable.add_behaviour(LampFadeOut(configurable, frames=30))
configurable.add_behaviour(WebListener(configurable))
configurable.wake()
