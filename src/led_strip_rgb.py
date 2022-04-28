import neopixel, machine
from time import sleep
import time
import uasyncio as asyncio
from easing import *

# Abstraction for light control - this gets used for the shade and base.
class LedStripRGB:
    def __init__(self, lamp, color, pin, num_pixels):
        self.lamp = lamp
        self.color = LedStripRGB.hex_to_rgb(color)
        self.num_pixels = num_pixels
        self.pin = pin
        self.lock = asyncio.Lock()

        self.pixels = neopixel.NeoPixel(machine.Pin(self.pin), self.num_pixels, bpp=3)
        self.default_pixels = [self.color] * self.num_pixels

    # Turn this LED Strip off
    def off(self):
        self.pixels.fill((0,0,0))
        self.pixels.write()

    # Reset to default
    def reset(self):
        self.update_colors(self.default_pixels)

   # set to a new color (tuple of rgb color or list of individual pixels)
    def fill(self, color):
        if isinstance(color, list):
            for i in range(self.num_pixels):
                self.pixels[i] = color[i]
        else:
            self.pixels.fill(color)

        self.pixels.write()

    # Shift pixels from their current state to a target state. Dest can be either a list of individual pixels or a RGB tuple
    # :TODO: Allow easing type to be passed
    async def async_fade(self, dest, steps, step_delay=1, background=False):
        if not isinstance(dest, list): dest = [dest] * self.num_pixels

        colors_start = list(self.pixels)

        colors = dict()
        for i in range(self.num_pixels):
            colors[i] = (
                QuadEaseInOut(start = colors_start[i][0], end = dest[i][0], duration = steps),
                QuadEaseInOut(start = colors_start[i][1], end = dest[i][1], duration = steps),
                QuadEaseInOut(start = colors_start[i][2], end = dest[i][2], duration = steps)
            )

        for step in range(steps):
            # If this  is a bakgrounded animation, lock the led strip while fading
            # so we don't try to animate two things at once. Otherwise,
            # we can asume things are not async (or handled) and not worry about it.
            if background: await self.lock.acquire()

            for p in range(self.num_pixels):
                self.pixels[p] = (
                    int(colors[p][0](step)),
                    int(colors[p][1](step)),
                    int(colors[p][2](step))
                )

            self.pixels.write()
            await asyncio.sleep_ms(step_delay)

            if background: self.lock.release()

    # Convert hex colors to RGB
    def hex_to_rgb(value):
        value = value.lstrip('#')
        return tuple(int(value[i:i+2], 16) for i in (0, 2, 4))