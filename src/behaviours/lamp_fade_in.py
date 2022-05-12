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
            #component will be in the stopped state twice: once on init and once above
            if self.animation_state == AnimationState.STOPPED and self.current_loop > 0:
                #land on the exact default pixel color
                self.lamp.base.buffer = self.lamp.base.default_pixels.copy()
                self.lamp.base.previous_buffer = self.lamp.base.default_pixels.copy()

                # run next behavior if configured
                for behaviour in self.chained_behaviors:
                    self.lamp.behaviour(behaviour).play()
                break

            await asyncio.sleep(0)
