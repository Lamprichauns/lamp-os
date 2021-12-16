import neopixel, machine
from time import sleep
import time
import uasyncio as asyncio


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

    #  Fade each pixel from it's current color to the target color by number of steps
    # :TODO: This should not be linear
    async def until_faded_to(self, color, steps = 50):
        for step in range(steps):
            steps_remaining = steps-step
            current_pixels = tuple(self.pixels)

            for pixel in range(self.num_pixels):
                px = current_pixels[pixel]

                red_diff = color[0] - px[0]
                green_diff = color[1] - px[1]
                blue_diff = color[2] - px[2]
                white_diff = color[3] - px[3]

                r = int(px[0] + (red_diff / steps_remaining))
                g = int(px[1] + (green_diff / steps_remaining))
                b = int(px[2] + (blue_diff / steps_remaining))
                w = int(px[3] + (white_diff / steps_remaining))

                self.pixels[pixel] = (r,g,b,w) # TODO: This is taking 0.6ms

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
