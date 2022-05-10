# default behaviors for all lamps
import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.fade import fade

class LampFadeIn(AnimatedBehaviour):
    async def draw(self):
        for i in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[i] = fade((0, 0, 0, 0), self.lamp.base.default_pixels[i], self.frame, self.frames)

        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = fade((0, 0, 0, 0), self.lamp.shade.default_pixels[i], self.frame, self.frames)

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
