import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour

class LampDmx(AnimatedBehaviour):
    def __init__(self, *args, dmx = None, **kwargs):
        super().__init__(*args, **kwargs)
        self.last_dmx_message = None
        self.lamp.dmx.add_update_callback(self.dmx_update)
        self.lamp.dmx.add_timeout_callback(self.dmx_timeout)

    def dmx_update(self, message):
        self.last_dmx_message = message

    def dmx_timeout(self):
        self.last_dmx_message = None

    async def draw(self):
        if self.last_dmx_message is not None:
            self.lamp.shade.buffer = [self.last_dmx_message[:4]] * self.lamp.shade.num_pixels
            self.lamp.base.buffer = [self.last_dmx_message[4:8]] * self.lamp.base.num_pixels

        await self.next_frame()

    async def control(self):
        while True:

            await asyncio.sleep(0)
