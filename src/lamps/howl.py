# An example of a lamp that can be setup using a web app
import re
import esp32
from ujson import load, dump
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.behaviour import BackgroundBehavior
from lamp_core.standard_lamp import StandardLamp
from utils.config import merge_configs
from utils.color_tools import darken
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_out import LampFadeOut
from behaviours.lamp_dimmer import LampDimmer
from vendor import tinyweb

# Define what we'll be setting in the web app
config = {
    "shade": { "pixels": 40, "color":"#220000", "pin": 12},
    "base": { "pixels": 40, "color":"#002200", "pin": 13},
    "lamp": { "name": "howl", "debug": False },
    "wifi": { "ssid": "howl-lamp" },
    "dimmer": { "baseline": 205, "threshold": 15, "amount": 50, "current": 0 }
}

# merge data from the database into the current config
# db is initially an empty json object to initialize the flash
with open("/lamps/files/howl/db", "r", encoding="utf8") as settings:
    merge_configs(config, load(settings))

# Start a standard lamp and extend it to be a Wifi Access Point
howl = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
howl.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

# To handle pastel colors and cool whites, we can darken the rgb values for a normal 40x40 lamp to not draw too much current
# tested with shade: #D486FE (pastel purple), base: #FF9494 (pastel rose) to get a 5V voltage of 4.8V
# This will ensure lamps can stay electrically stable for many hours even on extreme color settings
# This routine won't modify warm white values
for i in range(config["base"]["pixels"]):
    if howl.base.default_pixels[i][3] == 255:
        break

    howl.base.default_pixels[i] = darken(howl.base.default_pixels[i], 30)

for i in range(config["shade"]["pixels"]):
    if howl.shade.default_pixels[i][3] == 255:
        break

    howl.shade.default_pixels[i] = darken(howl.shade.default_pixels[i], 30)

howl.base.default_pixels = [(24,150,180,180)] * howl.base.num_pixels
howl.shade.default_pixels = [(0,0,0,180)] * howl.shade.num_pixels

# Web service init
app = tinyweb.webserver()

# Read and amend the config object
class Configurator():
    def get(self, _):
        config["dimmer"]["current"] = int(esp32.hall_sensor())
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
        config["dimmer"]["threshold"] = abs(int(number_sanitizer.sub("", data["threshold"])))
        config["dimmer"]["baseline"] = abs(int(number_sanitizer.sub("", data["baseline"])))
        config["dimmer"]["amount"] = abs(int(number_sanitizer.sub("", data["amount"])))

        if not config["lamp"]["name"]:
            return {'message': 'bad name'}, 500

        try:
            with open("/lamps/files/howl/db", "w", encoding="utf8") as flash:
                dump(config, flash)
        except Exception as e:
            print(e)

        howl.behaviour(LampFadeOut).play()

        return {'message': 'OK'}, 200

@app.route('/')
async def index(_, resp):
    await resp.send_file("/lamps/files/howl/configurator.html")

app.add_resource(Configurator, '/settings')

# Start listening for connections on port 80
class WebListener(BackgroundBehavior):
    async def run(self):
        await asyncio.sleep(5)
        app.run(host='0.0.0.0', port=80)

howl.add_behaviour(SocialGreeting(howl, frames=300))
howl.add_behaviour(LampFadeOut(howl, frames=30))
howl.add_behaviour(LampDimmer(howl, threshold=config["dimmer"]["threshold"], baseline=config["dimmer"]["baseline"], amount=config["dimmer"]["amount"], frames=15))
howl.add_behaviour(WebListener(howl))
howl.wake()
