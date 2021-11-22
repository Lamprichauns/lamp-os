import neopixel, machine
from gestures import Gestures
from time import sleep

# Abstraction for light control - this gets used for the shade and base.
class LedStrip(Gestures): 
    def __init__(self, color, pin, num_pixels):
        self.color = self.hex_to_rgb(color) 
        self.num_pixels = num_pixels
        self.pin = pin

        self.pixels = neopixel.NeoPixel(machine.Pin(self.pin), self.num_pixels, bpp=4, timing=1)

        # Turn everything off when initializing
        self.pixels.fill((0,0,0,0))
        self.pixels.write()
        sleep(0.5) 

    # Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
    # instead of each individual color)
    def hex_to_rgb(self, value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)