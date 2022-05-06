import neopixel
import machine
import uasyncio as asyncio
from vendor.easing import QuadEaseInOut

# Abstraction for light control - this gets used for the shade and base.
# Hardware support for Neopixels with Red, Green, Blue and White channels
# Prefer RGBWW (warm white) LED strips when purchasing hardware
class NeoPixelStrip:
    def __init__(self, color, pin, num_pixels, bpp=4):
        self.color = self.hex_to_rgbw(color)
        self.num_pixels = num_pixels
        self.pin = pin
        self.lock = asyncio.Lock()
        self.task = None
        self.bpp = bpp

        self.pixels = neopixel.NeoPixel(machine.Pin(self.pin), self.num_pixels, bpp=bpp)
        self.default_pixels = [self.color] * self.num_pixels

    # Turn this LED Strip off
    def off(self):
        self.fill((0,0,0,0))

    # Set to a new color (tuple of rgbw color or list of individual pixels)
    def fill(self, color):
        if isinstance(color, list):
            for i in range(self.num_pixels):
                self.pixels[i] = color[i]
        else:
            self.pixels.fill(color)

        self.pixels.write()

    # Shift pixels from their current state to a target state. Dest can be either a list of individual pixels or a RGBW tuple
    async def fade(self, dest, steps, step_delay=1, easing_function=QuadEaseInOut):
        if self.task:
            self.task.cancel()
            self.task = None
        print("fading")

        self.task = asyncio.create_task(self._fade(dest,steps,step_delay,easing_function))

    async def _fade(self, dest, steps, step_delay=1, easing_function=QuadEaseInOut):
        if not isinstance(dest, list):
            dest = [dest] * self.num_pixels

        colors_start = list(self.pixels)

        colors = {}
        for i in range(self.num_pixels):
            colors[i] = (
                easing_function(start = colors_start[i][0], end = dest[i][0], duration = steps),
                easing_function(start = colors_start[i][1], end = dest[i][1], duration = steps),
                easing_function(start = colors_start[i][2], end = dest[i][2], duration = steps),
                easing_function(start = colors_start[i][3], end = dest[i][3], duration = steps)
            )

        for step in range(steps):
            # Lock the led strip while fading so we don't try to animate two things at once
            async with self.lock:
                for p in range(self.num_pixels):
                    self.pixels[p] = (
                        int(colors[p][0](step)),
                        int(colors[p][1](step)),
                        int(colors[p][2](step)),
                        int(colors[p][3](step))
                    )

                self.pixels.write()
                await asyncio.sleep_ms(step_delay)

    # Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
    # instead of each individual color)
    def hex_to_rgbw(self, value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))

        if self.bpp == 4:
            return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)
        else:
            return rgb + (0,)