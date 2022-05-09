'''
NeoPixel driver based on the NeoPixel driver in MicroPython
Adds better support for RGB(W) types and for working with the
color types in this lib
'''

from machine import bitstream, Pin
from utils.helpers import timed_function

class ColorOrder:
    RGBW = (0, 1, 2, 3)
    RBGW = (0, 2, 1, 3)
    GBRW = (1, 2, 0, 3)
    GRBW = (1, 0, 2, 3)
    BRGW = (2, 0, 1, 3)
    BGRW = (2, 1, 0, 3)

class NeoPixel:
    TIMING_800kHz = (400, 850, 800, 450)
    TIMING_400hKz = (800, 1700, 1600, 900)

    def __init__(self, pin, n, bpp=4, color_order = ColorOrder.GRBW, timing=TIMING_800kHz):
        self.pin = Pin(pin)
        self.pin.init(self.pin.OUT)
        self.n = n
        self.bpp = bpp
        self.buffer = bytearray(n * bpp)
        self.previous_data = None
        self.timing = timing
        self.color_order = color_order

    # send bitmap to the led strip if there are changes
    def write(self, data):
        if data == self.previous_data:
            return

        self.previous_data = data

        if self.bpp == 3:
            for p in range(self.n):
                self.buffer[(p*3)] = data[p][1]
                self.buffer[(p*3)+1] = data[p][0]
                self.buffer[(p*3)+2] = data[p][2]

        if self.bpp == 4:
            for p in range(self.n):
                self.buffer[(p*4)] = data[p][1]
                self.buffer[(p*4)+1] = data[p][0]
                self.buffer[(p*4)+2] = data[p][2]
                self.buffer[(p*4)+3] = data[p][3]

        # BITSTREAM_TYPE_HIGH_LOW = 0
        bitstream(self.pin, 0, self.timing, self.buffer)
