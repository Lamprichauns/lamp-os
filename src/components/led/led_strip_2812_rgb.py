import neopixel
import machine
import uasyncio as asyncio
from vendor.easing import QuadEaseInOut

# @deprecated
# Abstraction for light control - this gets used for the shade and base.
# Hardware support for Neopixels with Red, Green and Blue channels
# These strips are not preferred for lamps and should only be used for development
class LedStrip2812RGB:
    def __init__(self, color, pin, num_pixels):
        self.color = self.hex_to_rgbw(color)
        self.num_pixels = num_pixels
        self.pin = pin
        self.pixels = neopixel.NeoPixel(machine.Pin(self.pin), self.num_pixels, bpp=3)
        self.default_pixels = [self.color] * self.num_pixels
        self.frame_buffer = [(0, 0, 0, 0)] * self.num_pixels

    # Turn this LED Strip off
    def off(self):
        self.fill((0,0,0,0))

    # Set to a new color (4-tuple of rgbw color or list of individual pixels)
    def fill(self, color):
        self.pixels.fill((color[0], color[1], color[2]))

    # Update a scene
    def draw(self, colors):
        self.frame_buffer = colors

    # Fetch the frame buffer
    def get_frame_buffer(self):
        return self.frame_buffer

    # Write the final scene to the LED strip
    def flush(self):
        for p in range(self.num_pixels):
            self.pixels[p] = (
                int(self.frame_buffer[p][0]),
                int(self.frame_buffer[p][1]),
                int(self.frame_buffer[p][2])
            )

        self.pixels.write()

    # Convert hex colors to RGBW
    def hex_to_rgbw(self, value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return rgb + (0,)
