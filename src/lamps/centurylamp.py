from lamp import Lamp
from behaviour import Behaviour
from motion_6050 import LampMotionMPU6050
import uasyncio as asyncio

config = {
    "base": { "pin": 12, "pixels": 40 },
    "shade": { "pin": 13, "pixels": 40 },
    "motion": { "pin_sda": 21, "pin_scl": 22, "polling_interval": 10e-2 }
}

motion = LampMotionMPU6050(config["motion"]["pin_sda"], config["motion"]["pin_scl"])

# Sample punter motion
class MotionCaptureBehavior(Behaviour):
    def measure(self):
        print(motion.get_movement_intensity_value())

    async def run(self):
        polling_interval = config["motion"]["polling_interval"]
        while True:
            await asyncio.sleep(polling_interval)
            async with self.lamp.lock:
                self.measure()

century = Lamp("century", "#5b4711", "#5b4711", config)
century.add_behaviour(MotionCaptureBehavior)
century.wake()
