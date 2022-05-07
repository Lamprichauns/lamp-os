# default behaviors for all lamps
import uasyncio as asyncio
from lamp_core.behaviour import StartupBehaviour
from utils.fade import fade

# Fade from off to the default colors on boot
class LampFadeIn(StartupBehaviour):
    async def fade(self, strip):
        start_color = (0, 0, 0, 0)
        end_color = strip.default_color

        # Blocking fade behaviour using the outer for loop
        for i in range(40):
            colors = {}

            for j in range(40):
                colors[j] = fade(start_color, end_color, i, 40)

            strip.buffer = colors
            strip.flush()

    async def run(self):
        base_fade = asyncio.create_task(self.fade(self.lamp.base))
        shade_fade = asyncio.create_task(self.fade(self.lamp.shade))
        await asyncio.gather(base_fade, shade_fade)
