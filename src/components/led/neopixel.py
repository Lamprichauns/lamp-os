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

    def __init__(self, pin, n, bpp=4, color_order = ColorOrder.GRBW, timing=TIMING_800kHz):
        self.pin = Pin(pin)
        self.pin.init(self.pin.OUT)
        self.n = n
        self.bpp = bpp
        self.buf = bytearray(n * bpp)
        self.timing = timing
        self.color_order = color_order

    def __len__(self):
        return self.n

    def __setitem__(self, i, v):
        offset = i * self.bpp
        for j in range(self.bpp):
            self.buf[offset + self.color_order[j]] = v[j]

    def __getitem__(self, i):
        offset = i * self.bpp
        return tuple(self.buf[offset + self.color_order[i]] for i in range(self.bpp))

    def write(self):
        # BITSTREAM_TYPE_HIGH_LOW = 0
        bitstream(self.pin, 0, self.timing, self.buf)
