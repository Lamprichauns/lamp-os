from lamp_core.behaviour import AnimatedBehaviour
from lamp_core.standard_lamp import StandardLamp
from behaviours.defaults import LampFadeIn
from utils.gradient import create_gradient
from utils.fade import fade, pingpong_fade
from vendor.easing import LinearInOut

# for ease of use, you can define a config to flow into all the components
config = {
    "base":  { "pin": 12, "pixels": 40, "bpp": 3},
    "shade": { "pin": 13, "pixels": 60, "bpp": 3},
    "touch": { "pin": 32 }
}

class WarpDrive(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.warp_drive_pattern = create_gradient((35, 7, 0, 0), (90, 23, 0, 0), self.lamp.shade.num_pixels)

    async def draw(self):
        warp_drive_pattern_start = self.warp_drive_pattern
        self.warp_drive_pattern = self.warp_drive_pattern[4:] + self.warp_drive_pattern[:4]

        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = fade(warp_drive_pattern_start[i], self.warp_drive_pattern[i], self.frame, self.frames, LinearInOut)

        await self.next_frame()

class WarningLights(AnimatedBehaviour):
    async def draw(self):
        colors = self.lamp.shade.buffer
        colors[18] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frame, self.frames)
        colors[10] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frame, self.frames)

        await self.next_frame()


animated_lamp = StandardLamp("crazybeans", "#5a1700", "#5a1700", config)
animated_lamp.add_behaviour(LampFadeIn(animated_lamp, frames=6, chained_behaviors=[WarpDrive, WarningLights]))
animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=20))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=10))
animated_lamp.wake()
