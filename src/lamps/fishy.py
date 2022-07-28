# A barebones lamp with configuration and social features
import random
import uasyncio as asyncio
from behaviours.social import SocialGreeting
from behaviours.lamp_fade_in import LampFadeIn
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import AnimatedBehaviour
from utils.fade import fade
from utils.gradient import create_gradient

# Override the standard lamp configs if necessary
config = {
    "base":  { "pin": 12 },
    "shade": { "pin": 14 },
    "lamp":  { "default_behaviours": False }
}
lamp = StandardLamp("fishy", "#ff000d", "#ff0000", config)

class BeaconCall(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    async def draw(self):
        percentage = pingpong_ease(0, 100, 0, self.frames, self.frame)

        for i in self.pixel_list:
            self.lamp.base.buffer[i] = brighten(self.lamp.base.buffer[i], percentage)

        await self.next_frame()


    async def control(self):
        while True:
            if self.animation_state in(AnimationState.PLAYING, AnimationState.STOPPING):



lamp.add_behaviour(LampFadeIn(lamp, frames=30, chained_behaviors=[BeaconCall]))
lamp.add_behaviour(BeaconCall(lamp, frames=20000))
lamp.add_behaviour(SocialGreeting(lamp, frames=3000))
lamp.wake()
