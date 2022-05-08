import random
import uasyncio as asyncio
from utils.gradient import create_gradient
from utils.fade import pingpong_fade
from lamp_core.behaviour import AnimatedBehaviour, AnimationState

class WarningLights(AnimatedBehaviour):
    async def draw(self):
        colors = self.lamp.shade.buffer
        colors[18] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frame, self.frames)
        colors[10] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frame, self.frames)

        self.lamp.shade.buffer = colors
        await self.next_frame()
