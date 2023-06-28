import re
from ujson import dump, load
import uasyncio as asyncio
from utils.config import merge_configs
from lamp_core.behaviour import BackgroundBehavior
from behaviours.lamp_fade_out import LampFadeOut
from vendor import tinyweb

# merge data from the database into the current config
# db is initially an empty json object to initialize the flash
def configurator_load_data(config):
    with open("/lamps/files/configurable/db", "r", encoding="utf8") as settings:
        merge_configs(config, load(settings))

    return config

# Read and amend the config object
class Router():
    def __init__(self, lamp, config):
        self.lamp = lamp
        self.config = config

    def get(self, _):
        return self.config

    def post(self, data):
        print(data)
        name_sanitizer = re.compile('[^a-z]')
        number_sanitizer = re.compile('[^0-9]+')
        self.config["shade"]["color"] = data["shade"]
        self.config["base"]["color"] = data["base"]
        self.config["shade"]["pixels"] = abs(int(number_sanitizer.sub("", data["shade_pixels"])))
        self.config["base"]["pixels"] = abs(int(number_sanitizer.sub("", data["base_pixels"])))
        self.config["lamp"]["name"] = name_sanitizer.sub("", data["name"])
        self.config["lamp"]["brightness"] = abs(int(number_sanitizer.sub("", data["brightness"])))
        self.config["lamp"]["home_mode"] = data.get("home_mode") == "on"

        if not self.config["lamp"]["name"]:
            return {'message': 'bad name'}, 500

        try:
            with open("/lamps/files/configurable/db", "w", encoding="utf8") as flash:
                dump(self.config, flash)
        except Exception as e:
            print(e)

        self.lamp.behaviour(LampFadeOut).play()

        return {'message': 'OK'}, 200

# Start listening for connections on port 80
class Configurator(BackgroundBehavior):
    def __init__(self, *args, config, **kwargs):
        super().__init__(*args, **kwargs)
        self.config = config
        self.lamp.add_behaviour(LampFadeOut(self.lamp, frames=100))

    async def run(self):
        app = tinyweb.webserver()

        @app.route('/')
        async def index(_, resp):
            await resp.send_file("/lamps/files/configurable/configurator.html")

        app.add_resource(Router(self.lamp, self.config), '/settings')

        await asyncio.sleep(5)
        app.run(host='0.0.0.0', port=80)
