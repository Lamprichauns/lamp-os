import neopixel, machine
from time import sleep
import uasyncio as asyncio

# Abstraction for light control - this gets used for the shade and base.
class LedStrip: 
    def __init__(self, color, pin, num_pixels):
        self.color = LedStrip.hex_to_rgb(color) 
        self.num_pixels = num_pixels
        self.pin = pin

        self.pixels = neopixel.NeoPixel(machine.Pin(self.pin), self.num_pixels, bpp=4, timing=1)

    # Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
    # instead of each individual color)
    def hex_to_rgb(value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)

    # Return a list of the colors between color_from and color_to in steps
    def colors_between(color_from, color_to, steps=100):
        red_diff = color_to[0] - color_from[0]
        green_diff = color_to[1] - color_from[1]
        blue_diff = color_to[2] - color_from[2]
        white_diff = color_to[3] - color_from[3]

        colors = []

        for i in range(steps):
            r = round(color_from[0] + (red_diff * i / steps))
            g = round(color_from[1] + (green_diff * i / steps))
            b = round(color_from[2] + (blue_diff * i / steps))
            w = round(color_from[3] + (white_diff * i / steps))

            colors.append((r,g,b,w))

        colors.append(color_to)
        return colors

    # Turn off the lights
    async def off(self): 
        self.pixels.fill((0,0,0,0))
        self.pixels.write()

    # Reset to the configured color
    async def reset(self):
        await asyncio.create_task(self.fade_to(self.color,20))

    # This is gross. I should learn python better!
    # This currently works but allocates way too much memory and is pretty terrible.
    # This should be able to be cleaned up to store less and loop less.
    # Probably replace steps with time in seconds when this is rewritten. 
    async def fade_to(self, color, steps = 50):
        colors = []

        for pixel in range(self.num_pixels):
            colors.append(LedStrip.colors_between(self.pixels[pixel], color, steps))

        for step in range(steps):
            # I tried to just put self.pixels[i] = colors[x][step] in the lambda, but python said no.
            pxls = list(map(lambda x: colors[x][step], range(self.num_pixels)))
            for i in range(self.num_pixels):
                self.pixels[i] = pxls[i]
            self.pixels.write()
            await asyncio.sleep_ms(100)