# default behaviors for all lamps
import uasyncio as asyncio
from lamp_core.behaviour import StartupBehaviour
from vendor.easing import QuadEaseInOut

# Fade from off to the default colors on boot
class LampFadeIn(StartupBehaviour):
    async def fade(self, strip):
        start_color = (0, 0, 0, 0)
        end_color = strip.default_pixels[0]

        # Blocking fade behaviour using the outer for loop
        for i in range(40):
            colors = {}

            for j in range(40):
                colors[j] = (
                    QuadEaseInOut(start_color[0], end_color[0], duration = 40)(i),
                    QuadEaseInOut(start_color[1], end_color[1], duration = 40)(i),
                    QuadEaseInOut(start_color[2], end_color[2], duration = 40)(i),
                    QuadEaseInOut(start_color[3], end_color[3], duration = 40)(i)
                )

            strip.draw(colors)
            strip.flush()

    async def run(self):
        self.lamp.base.off()
        self.lamp.shade.off()

        base_fade = asyncio.create_task(self.fade(self.lamp.base))
        shade_fade = asyncio.create_task(self.fade(self.lamp.shade))
        await asyncio.gather(base_fade, shade_fade)
