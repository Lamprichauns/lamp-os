import uasyncio as asyncio
import utime
from lamp_core.behaviour import AnimatedBehaviour
from utils.fade import fade

class LampDmx(AnimatedBehaviour):
    def __init__(self, *args, dmx = None, **kwargs):
        super().__init__(*args, **kwargs)
        self.last_dmx_message = None
        self.timeout = False
        self.revert = False
        self.lamp.dmx.add_update_callback(self.dmx_update)
        self.lamp.dmx.add_timeout_callback(self.dmx_timeout)

    def dmx_update(self, message):
        self.last_dmx_message = message

    def dmx_timeout(self):
        print("DMX Timeout")
        self.timeout = True

    async def draw(self):
        if self.last_dmx_message is not None and self.revert is False:
            self.lamp.shade.buffer = [self.last_dmx_message[:4]] * self.lamp.shade.num_pixels
            self.lamp.base.buffer = [self.last_dmx_message[4:8]] * self.lamp.base.num_pixels
        elif self.revert is True:
            for i in range(self.lamp.shade.num_pixels):
                self.lamp.shade.buffer[i] = fade(self.last_dmx_message[:4], self.lamp.shade.buffer[i], self.frames-1, self.frame)
            for i in range(self.lamp.base.num_pixels):
                self.lamp.base.buffer[i] = fade(self.last_dmx_message[4:8], self.lamp.base.buffer[i], self.frames-1, self.frame)
            if self.is_last_frame() and self.timeout == False:
                self.revert = False
                self.last_dmx_message = None

        await self.next_frame()

    async def control(self):
        while True:
            if self.timeout is True and self.frame == 0:
                self.revert = True
                self.timeout = False

            await asyncio.sleep(0)
