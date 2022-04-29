# default behaviors for all lamps
from lamp_core.behaviour import Behaviour
import uasyncio as asyncio

class LampFadeIn(Behaviour):
    async def run(self):
        self.lamp.base.off()
        self.lamp.shade.off()

        base_fade = asyncio.create_task(self.lamp.base.async_fade(self.lamp.base.default_pixels, 40))
        shade_fade = asyncio.create_task(self.lamp.shade.async_fade(self.lamp.shade.default_pixels, 40))

        async with self.lamp.lock:
            await asyncio.gather(base_fade, shade_fade)

class StartNetworking(Behaviour):
    async def run(self):
        async with self.lamp.lock:
            await self.lamp.bluetooth.enable()
            await self.lamp.network.start_monitoring()
