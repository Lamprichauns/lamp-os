import random
import uasyncio as asyncio
from utils.gradient import create_gradient
from utils.fade import fade, pingpong_fade
from lamp_core.behaviour import AnimatedBehaviour, ControllerBehaviour
from lamp_core.custom_lamp import CustomLamp
from lamp_core.frame_buffer import FrameBuffer
from components.led.neopixel import NeoPixel
from vendor.easing import LinearInOut
from behaviours.defaults import LampFadeIn

# for ease of use, you can define a config to flow into all the components
config = {
    "lamp": { "name": "custom" },
    "shade": { "pin": 13, "pixels": 40, "bpp": 3, "default_color": (90, 23, 0, 0) },
    "base": { "pin": 12, "pixels": 5, "bpp": 3, "default_color": (16, 20, 160, 0) },
}

animated_lamp = CustomLamp(config["lamp"]["name"])
animated_lamp.shade = FrameBuffer(config["shade"]["default_color"], config["shade"]["pixels"], NeoPixel(config["shade"]["pin"], config["shade"]["pixels"], config["shade"]["bpp"]))
animated_lamp.base = FrameBuffer(config["base"]["default_color"], config["base"]["pixels"], NeoPixel(config["base"]["pin"], config["base"]["pixels"], config["base"]["bpp"]))

class WarpDrive(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.warp_drive_pattern = create_gradient((35, 7, 0, 0), (90, 23, 0, 0), 40)

    async def run(self):
        warp_drive_pattern_start = self.warp_drive_pattern
        self.warp_drive_pattern = self.warp_drive_pattern[4:] + self.warp_drive_pattern[:4]
        colors = {}
        for i in range(40):
            colors[i] = fade(warp_drive_pattern_start[i], self.warp_drive_pattern[i], self.frame, self.frames, LinearInOut)

        self.lamp.shade.buffer = colors
        await self.next_frame()

class WarningLights(AnimatedBehaviour):
    async def run(self):
        colors = self.lamp.shade.buffer
        colors[18] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frame, self.frames)
        colors[10] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frame, self.frames)

        self.lamp.shade.buffer = colors
        await self.next_frame()

class ControllerMock(ControllerBehaviour):
    async def run(self):
        while True:
            chance = random.choice(range(0, 40))

            if chance > 35:
                print("pausing warning lights")
                self.lamp.behaviour(WarningLights).pause()

            if chance < 10:
                print("resuming warning lights")
                self.lamp.behaviour(WarningLights).resume()

            await asyncio.sleep(2)

class Draw(ControllerBehaviour):
    async def run(self):
        while True:
            self.lamp.shade.flush()
            await asyncio.sleep(0)

animated_lamp.add_behaviour(LampFadeIn(animated_lamp))
animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=30))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=40))
animated_lamp.add_behaviour(Draw(animated_lamp))
animated_lamp.add_behaviour(ControllerMock(animated_lamp))
animated_lamp.wake()
