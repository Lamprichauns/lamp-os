# default behaviors for all lamps
import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.fade import fade

class LampFadeIn(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.immediate_control = True

    async def draw(self):
        for i in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[i] = fade((0, 0, 0, 0), self.lamp.base.default_pixels[i], self.frames, self.frame)

        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = fade((0, 0, 0, 0), self.lamp.shade.default_pixels[i], self.frames, self.frame)

        await self.next_frame()

    async def control(self):
        self.play()
        self.stop()

        while True:
            #component will be in the stopped state twice: once on init and once above
            if self.animation_state == AnimationState.STOPPED and self.current_loop > 0:
                self.lamp.base.buffer = self.lamp.base.default_pixels.copy()
                self.lamp.base.previous_buffer = self.lamp.base.default_pixels.copy()

                for behaviour in self.chained_behaviors:
                    self.lamp.behaviour(behaviour).play()
                break

            await asyncio.sleep(0)
