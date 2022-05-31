# Fade out and reboot
from machine import reset
from lamp_core.behaviour import AnimatedBehaviour
from utils.fade import fade

class LampFadeOut(AnimatedBehaviour):
    async def draw(self):
        for i in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[i] = fade(self.lamp.base.buffer[i], (0, 0, 0, 0), self.frames, self.frame)

        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = fade(self.lamp.shade.buffer[i], (0, 0, 0, 0), self.frames, self.frame)

        if self.is_last_frame():
            self.stop()
            reset()

        await self.next_frame()
