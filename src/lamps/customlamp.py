# An example of a completely custom lamp
import random
from math import ceil
import uasyncio as asyncio
from behaviours.defaults import LampFadeIn
from lamp_core.custom_lamp import CustomLamp
from lamp_core.behaviour import Behaviour
from components.led.led_strip_6812_rgbww import LedStrip6812RGBWW
from components.motion.motion_6050 import MotionMPU6050
from components.network.bluetooth import Bluetooth

# for ease of use, you can define a config to flow into all the components
config = {
    "lamp": { "name": "custom" },
    "shade": { "pin": 13, "pixels": 40, "default_color": "#5b4711" },
    "base": { "pin": 12, "pixels": 5, "default_color": "#270000" },
    "motion": { "pin_sda": 21, "pin_scl": 22},
    "dance_reaction": {"polling_interval": 50, "dance_gesture_peak": 20000}
}

# Custom: Animate when lamp is under higher than normal acceleration to use as a rave scepter
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

# Compose components and behaviours to your liking
custom = CustomLamp(config["lamp"]["name"])
custom.shade = LedStrip6812RGBWW(config["shade"]["default_color"], config["shade"]["pin"], config["shade"]["pixels"])
custom.base = LedStrip6812RGBWW(config["base"]["default_color"], config["base"]["pin"], config["base"]["pixels"])
custom.motion = MotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])
custom.bluetooth = Bluetooth(config["lamp"]["name"], config["base"]["default_color"], config["shade"]["default_color"])
custom.network = custom.bluetooth.network
custom.bluetooth.enable()

custom.add_behaviour(LampFadeIn)
custom.add_behaviour(DanceReaction)

custom.wake()
