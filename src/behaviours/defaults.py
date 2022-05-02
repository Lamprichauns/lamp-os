# default behaviors for all lamps
import uasyncio as asyncio
from lamp_core.behaviour import Behaviour

# Fade from off to the default colors on boot
class LampFadeIn(Behaviour):
    async def run(self):
        async with self.lamp.lock:
            self.lamp.base.off()
            self.lamp.shade.off()

            base_fade = asyncio.create_task(self.lamp.base.fade(self.lamp.base.default_pixels, 40))
            shade_fade = asyncio.create_task(self.lamp.shade.fade(self.lamp.shade.default_pixels, 40))
            await asyncio.gather(base_fade, shade_fade)
