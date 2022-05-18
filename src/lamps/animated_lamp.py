from lamp_core.behaviour import AnimatedBehaviour
from lamp_core.standard_lamp import StandardLamp
from behaviours.lamp_fade_in import LampFadeIn
from utils.gradient import create_gradient
from utils.fade import pingpong_fade

# for ease of use, you can define a config to flow into all the components
config = {
    "base":  { "pin": 12, "pixels": 60, "bpp": 4},
    "shade": { "pin": 13, "pixels": 40, "bpp": 4},
    "lamp":  { "default_behaviours": False, "debug": True },
    "touch": { "pin": 32 }
}

class WarpDrive(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.warp_drive_pattern = create_gradient((35, 7, 0, 0), (90, 23, 0, 0), self.lamp.base.num_pixels)

    async def draw(self):
        self.warp_drive_pattern = self.warp_drive_pattern[5:] + self.warp_drive_pattern[:5]

        for i in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[i] = self.warp_drive_pattern[i]
        await self.next_frame()

class WarningLights(AnimatedBehaviour):
    async def draw(self):
        colors = self.lamp.base.buffer
        colors[18] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frames, self.frame)
        colors[10] = pingpong_fade(colors[10], (255, 255, 255, 0), colors[10], self.frames, self.frame)

        await self.next_frame()


animated_lamp = StandardLamp("crazybeans", "#5a1700", "#5a1700", config)

animated_lamp.base.default_pixels = create_gradient((233, 70, 0, 0), (250, 0, 0, 0), animated_lamp.base.num_pixels)
animated_lamp.add_behaviour(LampFadeIn(animated_lamp, frames=30, chained_behaviors=[WarpDrive, WarningLights]))
animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=60))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=60))
animated_lamp.wake()
