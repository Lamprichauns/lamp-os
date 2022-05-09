# default behaviors for all lamps
import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.fade import fade

class LampFadeIn(AnimatedBehaviour):
    async def draw(self):
        base_colors = self.lamp.base.buffer
        shade_colors = self.lamp.shade.buffer

        for j in range(self.lamp.base.num_pixels):
            base_colors[j] = fade((0, 0, 0, 0), self.lamp.base.default_color, self.frame, self.frames)

        for j in range(self.lamp.shade.num_pixels):
            shade_colors[j] = fade((0, 0, 0, 0), self.lamp.shade.default_color, self.frame, self.frames)

        self.lamp.base.buffer = base_colors
        self.lamp.shade.buffer = shade_colors
        await self.next_frame()

    async def control(self):
        self.lamp.behaviour(LampFadeIn).play()
        self.lamp.behaviour(LampFadeIn).stop()

        while True:
            if self.lamp.behaviour(LampFadeIn).animation_state == AnimationState.STOPPED:
                for behavior in self.chained_behaviors:
                    self.lamp.behaviour(behavior).play()
                break

            await asyncio.sleep(0)
