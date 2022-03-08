from .rgb import *
from .rgbw import *

# Abstract base class for Pixel List types, should never be used directly
class PixelList:
    def __init__(self):
        self._size = 0
        self._pixels = []

    @property
    def bit_per_pixel(self):
        return 0

    def pack(self, bytes, order):
        pass

    def __len__(self):
        return self._size

    def __getitem__(self, i):
        return self._pixels[i]

    def __setitem__(self, i, val):
        self._pixels[i] = val

    @property
    def size(self):
        return self._size

    def fill(self, color):
        for i in range(len(self._pixels)):
            self._pixels[i] = color

    def fade(self, scale):
        for pixel in self._pixels:
            pixel.fade(scale)

class RGBList(PixelList):
    def __init__(self, size):
        self._size = size
        self._pixels = [RGB() for i in range(size)]

    @property
    def bit_per_pixel(self):
        return 3

    def pack(self, bytes, order):
        for i in range(self.size):
            offset = i * 3
            bytes[offset + order[0]] = self._pixels[i].r
            bytes[offset + order[1]] = self._pixels[i].g
            bytes[offset + order[2]] = self._pixels[i].b

class RGBWList(PixelList):
    def __init__(self, size):
        self._size = size
        self._pixels = [RGBW() for i in range(size)]

    @property
    def bit_per_pixel(self):
        return 4

    def pack(self, bytes, order):
        for i in range(self.size):
            offset = i * 4
            bytes[offset + order[0]] = self._pixels[i].r
            bytes[offset + order[1]] = self._pixels[i].g
            bytes[offset + order[2]] = self._pixels[i].b
            bytes[offset + order[3]] = self._pixels[i].w
