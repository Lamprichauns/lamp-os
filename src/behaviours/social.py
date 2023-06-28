import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from utils.fade import fade

class SocialGreeting(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arrived = None
        self.frames = self.frames or 1200
        self.ease_frames = 60
        self.use_in_home_mode = False

    async def draw(self):
        for i in range(self.lamp.shade.num_pixels):
            if self.frame < self.ease_frames:
                self.lamp.shade.buffer[i] = fade(self.lamp.shade.buffer[i], self.arrived["base_color"], self.ease_frames, self.frame)

            elif self.frame > self.frames-self.ease_frames:
                self.lamp.shade.buffer[i] = fade(self.arrived["base_color"], self.lamp.shade.buffer[i], self.ease_frames, self.frame % self.ease_frames)

            else:
                self.lamp.shade.buffer[i] = self.arrived["base_color"]
        await self.next_frame()

    async def control(self):
        while True:
            # wait for lamp to be started up for a while on first boot
            await asyncio.sleep(15)

            if self.animation_state not in (AnimationState.PLAYING, AnimationState.STOPPING):
                arrived = await self.lamp.network.arrived()
                self.arrived = arrived
                print("%s has arrived" % (arrived["name"]))
                self.play()
                self.stop()
