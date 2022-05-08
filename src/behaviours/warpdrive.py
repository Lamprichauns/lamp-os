from utils.gradient import create_gradient
from utils.fade import fade
from lamp_core.behaviour import AnimatedBehaviour
from vendor.easing import LinearInOut

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
