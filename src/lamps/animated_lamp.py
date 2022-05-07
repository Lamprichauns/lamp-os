import random
import uasyncio as asyncio
from utils.gradient import create_gradient
from utils.fade import fade, pingpong_fade
from lamp_core.behaviour import AnimatedBehaviour, ControllerBehaviour
from lamp_core.standard_lamp import StandardLamp
from vendor.easing import LinearInOut

# for ease of use, you can define a config to flow into all the components
config = {
    "base":  { "pin": 12, "pixels": 40, "bpp": 3},
    "shade": { "pin": 13, "pixels": 40, "bpp": 3},
    "touch": { "pin": 32 }
}

animated_lamp = StandardLamp("crazybeans", "#5a1700", "#5a1700", config)

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

animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=30))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=40))
animated_lamp.add_behaviour(Draw(animated_lamp))
animated_lamp.add_behaviour(ControllerMock(animated_lamp))
animated_lamp.wake()
