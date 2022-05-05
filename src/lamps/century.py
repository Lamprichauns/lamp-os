# Century lamp
import random
from math import ceil
import uasyncio as asyncio
from behaviours.defaults import LampFadeIn
from behaviours.social import SocialGreeting
from lamp_core.custom_lamp import CustomLamp
from lamp_core.behaviour import Behaviour
from components.led.led_strip_2812_rgb import LedStrip2812RGB
from components.motion.motion_6050 import MotionMPU6050
from components.network.access_point import AccessPoint
from components.network.bluetooth import Bluetooth
from components.temperature.temperature_6050 import TemperatureMPU6050
from utils.color_gradient import create_gradient
from utils.color_brighten import brighten
from utils.color_darken import darken
from vendor import tinyweb

# for ease of use, you can define a config to flow into all the components
config = {
    "lamp": { "name": "century" },
    "shade": { "pin": 12, "pixels": 5, "default_color": "#931702" },
    "base": { "pin": 13, "pixels": 60, "default_color": "#270000" },
    "motion": { "pin_sda": 21, "pin_scl": 22},
    "dance_reaction": {"polling_interval": 50, "dance_gesture_peak": 20000},
    "sunset": {"temperature_low": 34, "temperature_high": 40 },
    "webapp": {"ssid": "century-lamp", "password": "123456789"}
}

# Compose all the components
century = CustomLamp(config["lamp"]["name"])
century.shade = LedStrip2812RGB(config["shade"]["default_color"], config["shade"]["pin"], config["shade"]["pixels"])
century.base = LedStrip2812RGB(config["base"]["default_color"], config["base"]["pin"], config["base"]["pixels"])
century.motion = MotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])
century.temperature = TemperatureMPU6050(century.motion.accelerometer)
century.bluetooth = Bluetooth(config["lamp"]["name"], config["base"]["default_color"], config["shade"]["default_color"])
century.network = century.bluetooth.network
century.bluetooth.enable()
century.access_point = AccessPoint(ssid=config["webapp"]["ssid"], password=config["webapp"]["password"])

# Setup first sunset scene
century.base.default_pixels = create_gradient((158, 10, 3, 0), (220, 80, 8, 0), steps=config["base"]["pixels"])

for j in range(32,37):
    century.base.default_pixels[j] = darken(century.base.default_pixels[j], percentage=85)
for j in range(20,25):
    century.base.default_pixels[j] = brighten(century.base.default_pixels[j], percentage=15)

# Kick off the web server for lamp remote controls
app = tinyweb.webserver()

# Handle new values
class Configurator():
    def post(self, data):
        config["sunset"]["temperature_low"] = int(data["temperature_low"])
        config["sunset"]["temperature_high"] = int(data["temperature_high"])
        print(data)

        return {'message': 'OK'}, 200

def build_options(selected = 0):
    options = ""
    for i in range(0, 45):
        options += """<option value="{value}" {selected}>{text}</option>""".format(value=i, selected="selected=selected" if i == selected else "", text=i)

    return options

# Index page
@app.route('/')
async def index(_, response):

    await response.start_html()
    await response.send("""
<html>
    <h1>LampOS</h1>
    <h2>Current temperature is: {temperature}</h2>
    <form action="/settings" method="post">
        <label for="temperature_low">Low Temperature Threshold</label>
        <select id="temperature_low" name="temperature_low">{temperature_low_options}</select><br/>
        <label for="temperature_high">High Temperature Threshold</label>
        <select id="temperature_high" name="temperature_high">{temperature_high_options}</select><br/>
        <input type="submit" value="Submit">
    </form>
    <h3><3</h3>
</html>
    """.format(
        temperature=century.temperature.get_temperature_value(),
        temperature_low_options=build_options(config["sunset"]["temperature_low"]),
        temperature_high_options=build_options(config["sunset"]["temperature_high"])
        )
    )

app.add_resource(Configurator, '/settings')

# Start listening for connections on port 80
class WebListener(Behaviour):
    async def run(self):
        app.run(host='0.0.0.0', port=80)

# This behavior will create a very slow cooling/warming color change in reaction the ambient temperature
class Sunset(Behaviour):
    def __init__(self, lamp):
        super().__init__(lamp)
        self.sunset_stages = [
            { "color_start":(150, 10, 0, 0), "color_end": (220, 80, 8, 0), "sun": True, "clouds": False, "stars": False, "temperature_threshold": 40 },
            { "color_start":(108, 13, 3, 0), "color_end": (217, 68, 30, 0), "sun": True, "clouds": False, "stars": False, "temperature_threshold": 39 },
            { "color_start":(108, 4, 23, 0), "color_end": (181, 96, 14, 0), "sun": True, "clouds": True, "stars": False, "temperature_threshold": 38 },
            { "color_start":(107, 5, 57, 0), "color_end": (155, 140, 30, 0), "sun": False, "clouds": True, "stars": False, "temperature_threshold": 37 },
            { "color_start":(52, 4, 107, 0), "color_end": (104, 180, 15, 0), "sun": False, "clouds": True, "stars": True, "temperature_threshold": 36 },
            { "color_start":(16, 7, 142, 0), "color_end": (85, 115, 16, 0), "sun": False, "clouds": True, "stars": True, "temperature_threshold": 34 },
            { "color_start": (5, 33, 90, 0), "color_end": (30, 90, 12, 0), "sun": False, "clouds": True, "stars": True, "temperature_threshold": 0 },
        ]
        self.current_scene = 0
        self.polling_interval = 30

    def get_scene_for_temperature(self):
        temperature = self.lamp.temperature.get_temperature_value()
        print(temperature)

        for i, k in enumerate(self.sunset_stages):
            if temperature > k["temperature_threshold"]:
                return i

        return 0

    def draw_scene(self, scene):
        new_pixels = create_gradient(self.sunset_stages[scene]["color_start"], self.sunset_stages[scene]["color_end"], steps=config["base"]["pixels"])

        if self.sunset_stages[scene]["sun"] is True:
            new_pixels[random.choice(range(5, 20))] = (250, 90, 0, 0)

        if self.sunset_stages[scene]["clouds"] is True:
            new_pixels[random.choice(range(19, 28))] = (255, 0, 210, 0)
            new_pixels[random.choice(range(18, 25))] = (230, 0, 110, 0)
            new_pixels[random.choice(range(20, 27))] = (212, 0, 178, 0)
            new_pixels[random.choice(range(43, 57))] = (212, 0, 178, 0)
            new_pixels[random.choice(range(48, 60))] = (212, 0, 178, 0)

        if self.sunset_stages[scene]["stars"] is True:
            new_pixels[random.choice(range(25, 40))] = (250, 250, 250, 0)
            new_pixels[random.choice(range(27, 40))] = (250, 250, 250, 0)
            new_pixels[random.choice(range(40, 50))] = (250, 250, 250, 0)
            new_pixels[random.choice(range(50, 60))] = (250, 250, 250, 0)

        for k in range(32,37):
            new_pixels[k] = darken(self.lamp.base.default_pixels[k], percentage=85)

        for k in range(18,23):
            new_pixels[k] = brighten(self.lamp.base.default_pixels[k], percentage=15)

        return new_pixels

    async def run(self):
        while True:
            await asyncio.sleep(self.polling_interval)
            scene = self.get_scene_for_temperature()

            print("Rotating sun/stars/clouds")
            await self.lamp.base.fade(self.draw_scene(self.current_scene), 100, 300)

            if scene != self.current_scene:
                print("Scene change")
                self.current_scene = scene
                await self.lamp.base.fade(self.draw_scene(scene), 150, 500)

# Animate when lamp is under higher than normal acceleration to use as a rave scepter
class DanceReaction(Behaviour):
    def __init__(self, lamp):
        super().__init__(lamp)
        self.last_accelerometer_value = 0
        self.polling_interval = config["dance_reaction"]["polling_interval"]
        self.dance_gesture_peak = config["dance_reaction"]["dance_gesture_peak"]

    async def measure(self):
        value = self.lamp.motion.get_movement_intensity_value()
        if (value >= self.dance_gesture_peak and value is not self.last_accelerometer_value):
            self.last_accelerometer_value = value
            pixel_list = [random.randrange(0, self.lamp.shade.num_pixels, 1) for i in range(ceil(self.lamp.shade.num_pixels/2))]
            current_pixels = list(self.lamp.shade.pixels)
            new_pixels = current_pixels.copy()

            # Quick flash to white
            for i in pixel_list:
                new_pixels[i] = (250, 250, 255, 255)
            await self.lamp.shade.fade(new_pixels, 10, 2)

            # Quick flash to black
            for i in pixel_list:
                new_pixels[i] = (0, 0, 0, 0)
            await self.lamp.shade.fade(new_pixels, 5, 5)

            # Slow fade back to before
            await self.lamp.shade.fade(current_pixels, 10, 5)

            # Don't trigger again for a while
            await asyncio.sleep_ms(500)

    async def run(self):
        while True:
            await asyncio.sleep_ms(self.polling_interval)
            async with self.lamp.lock:
                await self.measure()

century.add_behaviour(LampFadeIn)
century.add_behaviour(DanceReaction)
century.add_behaviour(Sunset)
century.add_behaviour(WebListener)
century.add_behaviour(SocialGreeting)

century.wake()
