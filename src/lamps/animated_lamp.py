import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from lamp_core.standard_lamp import StandardLamp
from behaviours.lamp_fade_in import LampFadeIn
from utils.gradient import create_gradient
from utils.fade import pingpong_fade

# This is an animation test harness to validate the features work

config = {
    "base":  { "pin": 14, "pixels": 40, "bpp": 4 },
    "shade": { "pin": 12, "pixels": 40, "bpp": 4 },
    "lamp":  { "default_behaviours": False },
    "touch": { "pin": 32 }
}

# Creates a fast-moving gradient background
class WarpDrive(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.warp_drive_pattern = create_gradient((35, 7, 0, 0), (90, 23, 0, 0), self.lamp.base.num_pixels)

    async def draw(self):
        self.warp_drive_pattern = self.warp_drive_pattern[5:] + self.warp_drive_pattern[:5]

        for i in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[i] = self.warp_drive_pattern[i]
        await self.next_frame()

# Flashes LEDs 18 and 10 at a rapid pace
class WarningLights(AnimatedBehaviour):
    async def draw(self):
        colors = self.lamp.base.buffer
        colors[18] = pingpong_fade(colors[18], (255, 255, 255, 0), colors[18], self.frames, self.frame)
        colors[10] = pingpong_fade(colors[10], (255, 255, 255, 0), colors[10], self.frames, self.frame)

        await self.next_frame()

# Tests the ability to freeze blinking red LEDs on a particular frame
class PauseDemo(AnimatedBehaviour):
    async def draw(self):
        colors = self.lamp.base.buffer
        colors[17] = pingpong_fade(colors[17], (255, 0, 0 , 0), colors[17], self.frames, self.frame)
        colors[9] = pingpong_fade(colors[9], (255, 0, 0, 0), colors[9], self.frames, self.frame)

        await self.next_frame()

    async def control(self):
        ticks = 0

        while True:
            if ticks % 65 == 0:
                if self.animation_state == AnimationState.PLAYING:
                    self.pause()
                else:
                    self.play()

            ticks += 1

            await asyncio.sleep(0)


animated_lamp = StandardLamp("crazybeans", "#5a1700", "#5a1700", config)
animated_lamp.base.default_pixels = create_gradient((233, 70, 0, 0), (250, 0, 0, 0), animated_lamp.base.num_pixels)
animated_lamp.add_behaviour(LampFadeIn(animated_lamp, frames=30, chained_behaviors=[WarpDrive]))
animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=60))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=60, auto_play=True))
animated_lamp.add_behaviour(PauseDemo(animated_lamp, frames=33, auto_play=True))
animated_lamp.wake()
