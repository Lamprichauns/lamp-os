# An example of a completely custom lamp
from lamp_core.custom_lamp import CustomLamp
from lamp_core.behaviour import Behaviour
from components.led.led_strip_2812_rgb import LedStrip2812RGB
from components.motion.motion_6050 import MotionMPU6050
from behaviours.defaults import LampFadeIn
from math import ceil
import uasyncio as asyncio
import random

config = {
    "shade": { "pin": 13, "pixels": 40, "default_color": "#5b4711" },
    "base": { "pin": 12, "pixels": 5, "default_color": "#270000" },
    "motion": { "pin_sda": 21, "pin_scl": 22},
    "dance_reaction": {"polling_interval": 50, "dance_gesture_peak": 20000}
}

# Custom: Animate when lamp is under higher than normal acceleration to try and hype
class DanceReaction(Behaviour):
    async def measure(self):
        value = self.lamp.motion.get_movement_intensity_value()
        if (value >= self.dance_gesture_peak):
            pixel_list = [random.randrange(0, self.lamp.shade.num_pixels, 1) for i in range(ceil(self.lamp.shade.num_pixels/2))]
            current_pixels = list(self.lamp.shade.pixels)
            new_pixels = current_pixels.copy()

            # Quick flash to white
            for i in pixel_list:
                new_pixels[i] = (250, 250, 255)
            await self.lamp.shade.async_fade(new_pixels, 10, 2)

            # Quick flash to black
            for i in pixel_list:
                new_pixels[i] = (0, 0, 0)
            await self.lamp.shade.async_fade(new_pixels, 5, 5)

            # Slow fade back to before
            await self.lamp.shade.async_fade(current_pixels, 10, 5)

    async def run(self):
        polling_interval = config["dance_reaction"]["polling_interval"]
        self.dance_gesture_peak = config["dance_reaction"]["dance_gesture_peak"];
        while True:
            await asyncio.sleep_ms(polling_interval)
            async with self.lamp.lock:
                await self.measure()

# Compose components and behaviours
century = CustomLamp("century")
century.shade = LedStrip2812RGB(century, config['shade']['default_color'], config['shade']['pin'], config['shade']['pixels'])
century.base = LedStrip2812RGB(century, config['base']['default_color'], config['base']['pin'], config['base']['pixels'])
century.motion = MotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])

century.add_behaviour(LampFadeIn)
century.add_behaviour(DanceReaction)

century.wake()
