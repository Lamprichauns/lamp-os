from .fastmath import *
from .hsv import *

class RGB:
    def __init__(self, r=0, g=0, b=0):
        self.r = r
        self.g = g
        self.b = b

    def __repr__(self) -> str:
        return f'RGB({self.r}, {self.g}, {self.b})'

    def __len__(self):
        return 3

    def __getitem__(self, i):
        if i == 0:
            return self.r
        if i == 1:
            return self.g
        if i == 2:
            return self.b

    @classmethod
    def from_(cls, val):
        if isinstance(val, RGB):
            return cls.from_rgb(val)
        if isinstance(val, HSV):
            return cls.from_hsv(val)
        if isinstance(val, tuple):
            return cls.from_tuple(val)
        if isinstance(val, int):
            return cls.from_code(val)
        if isinstance(val, str):
            return cls.from_hex_string(val)

    @classmethod
    def from_code(cls, colorcode):
        r = (colorcode >> 16) & 0xff
        g = (colorcode >>  8) & 0xff
        b = (colorcode      ) & 0xff
        return cls(r, g, b)

    @classmethod
    def from_rgb(cls, val):
        return cls(val.r, val.g, val.b)

    @classmethod
    def from_hsv(cls, val):
        return cls(*val.to_raw_rgb())

    @classmethod
    def from_tuple(cls, val):
        return cls(val[0], val[1], val[2])

    @classmethod
    def from_hex_string(cls, val):
        val = val.lstrip('#')
        rgb = tuple(int(val[i : i + 2], 16) for i in (0, 2, 4))
        return cls.from_tuple(rgb)

    def __iadd__(self, rhs):
        self.r = qadd8(self.r, rhs.r)
        self.g = qadd8(self.g, rhs.g)
        self.b = qadd8(self.b, rhs.b)
        return self

    def __add__(self, rhs):
        ret = RGB.from_rgb(self)
        ret += rhs
        return ret

    def __isub__(self, rhs):
        self.r = qsub8(self.r, rhs.r)
        self.g = qsub8(self.g, rhs.g)
        self.b = qsub8(self.b, rhs.b)
        return self

    def __sub__(self, rhs):
        ret = RGB.from_rgb(self)
        ret -= rhs
        return ret

    def fade(self, scale):
        self.r, self.g, self.b = nscale8x3(self.r, self.g, self.b, scale)

    def fade_copy(self, scale):
        ret = RGB.from_rgb(self)
        ret.fade(scale)
        return ret

    # --------- Component Properties ---------

    @property
    def raw(self):
        return (self.r, self.g, self.b)

    @property
    def red(self):
        return self.r

    @red.setter
    def red(self, value):
        self.r = value

    @property
    def green(self):
        return self.g

    @green.setter
    def green(self, value):
        self.g = value

    @property
    def blue(self):
        return self.b

    @blue.setter
    def blue(self, value):
        self.b = value
