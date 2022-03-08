'''
NeoPixel driver based on the NeoPixel driver in MicroPython
Adds better support for RGB(W) types and for working with the
color types in this lib
'''

from machine import bitstream, Pin

class ColorOrder:
    RGB  = (0, 1, 2)
    RBG  = (0, 2, 1)
    GBR  = (1, 2, 0)
    GRB  = (1, 0, 2)
    BRG  = (2, 0, 1)
    BGR  = (2, 1, 0)

    RGBW = (0, 1, 2, 3)
    RBGW = (0, 2, 1, 3)
    GBRW = (1, 2, 0, 3)
    GRBW = (1, 0, 2, 3)
    BRGW = (2, 0, 1, 3)
    BGRW = (2, 1, 0, 3)

class NeoPixel:
    TIMING_800kHz = (400, 850, 800, 450)
    TIMING_400hKz = (800, 1700, 1600, 900)

    def __init__(self, pin, pixels, color_order=ColorOrder.RGB, timing=TIMING_800kHz):
        self.pin = Pin(pin)
        self.pixels = pixels
        self.color_order = color_order
        self.timing = self.TIMING_800kHz

        self.buffer = bytearray(len(pixels) * pixels.bit_per_pixel)
        self.pin.init(self.pin.OUT)

    def __len__(self):
        return len(self.pixels)

    def write(self):
        self.pixels.pack(self.buffer, self.color_order)
        bitstream(self.pin, 0, self.timing, self.buffer)
