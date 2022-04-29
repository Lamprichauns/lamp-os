from lamp import Lamp
from behaviour import Behaviour
from motion_6050 import LampMotionMPU6050
import uasyncio as asyncio
import random

config = {
    "base": { "pin": 12, "pixels": 40 },
    "shade": { "pin": 13, "pixels": 40 },
    "motion": { "pin_sda": 21, "pin_scl": 22, "polling_interval": 10e-2, "dance_gesture_peak": 23000 }
}

motion = LampMotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])

# Capture motion of the lamp and react to events like dancing
class MotionCapture(Behaviour):
    async def measure(self):
        value = motion.get_movement_intensity_value()
        if (value >= self.dance_gesture_peak):
            print("dancing");
            pixel = random.choice(range(0,self.lamp.shade.num_pixels-10))
            pixel2 = random.choice(range(0,self.lamp.shade.num_pixels-10))

            current_pixels = list(self.lamp.shade.pixels)

            new_pixels = current_pixels.copy()

            # Quick flash to white
            new_pixels[pixel] = (250,250,255)
            new_pixels[pixel2] = (250,250,255)
            await self.lamp.shade.async_fade(new_pixels,10,2)

            # Quick flash to black
            new_pixels[pixel] = (0,0,0)
            new_pixels[pixel2] = (0,0,0)
            await self.lamp.shade.async_fade(new_pixels,20,5)

            # Slow fade back to before
            await self.lamp.shade.async_fade(current_pixels,20,5)

    async def run(self):
        polling_interval = config["motion"]["polling_interval"]
        self.dance_gesture_peak = config["motion"]["dance_gesture_peak"];
        while True:
            await asyncio.sleep(polling_interval)
            async with self.lamp.lock:
                await self.measure()

century = Lamp("century", "#5b4711", "#5b4711", config)
century.add_behaviour(MotionCapture)
century.wake()
