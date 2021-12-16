import neopixel, machine
from time import sleep
import time
import uasyncio as asyncio
from easing import *

# :TODO: Unsure on naming of this mixin/class. Also once that's settled, move to another file.
class LedGestures:
    # set to a new color
    async def until_color_changed(self, color): 
        self.pixels.fill(color)
        self.pixels.write()

    # Turn off the lights
    async def until_off(self): 
        await self.until_color_changed((0,0,0,0))

    # Reset to the configured color
    async def until_reset(self):
        await self.until_color_changed(self.color)


    async def until_faded_to(self, color, steps):
        colors_start = tuple(self.pixels)
        
        colors = dict()
        for i in range(self.num_pixels):
            colors[i] = (
                CubicEaseOut(start = colors_start[i][0], end = color[0], duration = steps),
                CubicEaseOut(start = colors_start[i][1], end = color[1], duration = steps),
                CubicEaseOut(start = colors_start[i][2], end = color[2], duration = steps),
                CubicEaseOut(start = colors_start[i][3], end = color[3], duration = steps)
            )

        for step in range(steps): 
            for p in range(self.num_pixels):
                self.pixels[p] = (
                    int(colors[p][0](step)),
                    int(colors[p][1](step)),
                    int(colors[p][2](step)),
                    int(colors[p][3](step))
                )

            self.pixels.write()  
            await asyncio.sleep_ms(1)

# Abstraction for light control - this gets used for the shade and base.
class LedStrip(LedGestures): 
    def __init__(self, color, pin, num_pixels):
        self.color = LedStrip.hex_to_rgb(color) 
        self.num_pixels = num_pixels
        self.pin = pin

        self.pixels = neopixel.NeoPixel(machine.Pin(self.pin), self.num_pixels, bpp=4)

    # Reset this LED Strip to it's default color
    def reset(self): 
        await self.until_reset()

    # Turn this LED Strip off
    def off(self):
        await self.until_off()

    # Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
    # instead of each individual color)
    def hex_to_rgb(value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)
