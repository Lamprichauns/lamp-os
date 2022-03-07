from .fastmath import *
from .rgb import *
from .hsv import *

class RGBW:
    def __init__(self, r=0, g=0, b=0, w=0):
        self.r = r
        self.g = g
        self.b = b
        self.w = w

    def __repr__(self) -> str:
        return f'RGBW({self.r}, {self.g}, {self.b}, {self.w})'

    def __len__(self):
        return 4

    def __getitem__(self, i):
        if i == 0:
            return self.r
        if i == 1:
            return self.g
        if i == 2:
            return self.b
        if i == 3:
            return self.w

    @classmethod
    def from_(cls, val):
        if isinstance(val, RGB):
            return cls.from_rgb(val)
        if isinstance(val, RGBW):
            return cls.from_rgbw(val)
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
        r = (colorcode >> 24) & 0xff
        g = (colorcode >> 16) & 0xff
        b = (colorcode >>  8) & 0xff
        w = (colorcode      ) & 0xff
        return cls(r, g, b, w)

    @classmethod
    def from_rgb(cls, val):
        return cls(val.r, val.g, val.b)

    # Convert RGB to RGBW.  Simply finds the common white in the 
    # `r`, `g`, `b` components and places that in `white` then
    # decrements the components.
    #
    # Eg. RGB(100, 80, 60) -> RGBW(40, 20, 0, 60)
    # 
    # !!! Experimental !!!
    #  `brightness_boost` my weird attempt at better leveraging RGBW
    # strips with RGB values.  This will leave a percentage of the 
    # original component value in while still adding it to white.
    # `brightness_boost` range: 0 - 255. 255 being full boost, 0 being 
    # none. The scale is linear.
    #
    # Eg. RGB(100, 80, 60) with boost = 127 -> RGBW(80, 60, 20, 60)
    @classmethod
    def remap_rgb(cls, val, brightness_boost=0):
        w = min(val.r, val.g, val.b)

        boost = scale8(w, brightness_boost)

        r = (val.r - w) + boost
        g = (val.g - w) + boost
        b = (val.b - w) + boost

        return cls(r, g, b, w)

    @classmethod
    def from_rgbw(cls, val):
        return cls(val.r, val.g, val.b, val.w)

    @classmethod
    def from_hsv(cls, val):
        return cls(*val.to_raw_rgb())

    @classmethod
    def remap_hsv(cls, val, brightness_boost=0):
        return cls.remap_rgb(RGB.from_hsv(val), brightness_boost)

    @classmethod
    def from_tuple(cls, val):
        return cls(val[0], val[1], val[2], val[3])

    @classmethod
    def from_hex_string(cls, val):
        if len(val) == 7:
            return cls.from_rgb(RGB.from_hex_string(val))

        val = val.lstrip('#')
        rgbw = tuple(int(val[i : i + 2], 16) for i in (0, 2, 4, 6))
        return cls.from_tuple(rgbw)

    def __iadd__(self, rhs):
        self.r = qadd8(self.r, rhs.r)
        self.g = qadd8(self.g, rhs.g)
        self.b = qadd8(self.b, rhs.b)
        self.w = qadd8(self.w, rhs.w)
        return self

    def __add__(self, rhs):
        ret = RGBW.from_rgbw(self)
        ret += rhs
        return ret

    def __isub__(self, rhs):
        self.r = qsub8(self.r, rhs.r)
        self.g = qsub8(self.g, rhs.g)
        self.b = qsub8(self.b, rhs.b)
        self.w = qsub8(self.w, rhs.w)
        return self

    def __sub__(self, rhs):
        ret = RGBW.from_rgbw(self)
        ret -= rhs
        return ret

    def fade(self, scale):
        self.r, self.g, self.b, self.w = nscale8x4(self.r, self.g, self.b, self.w, scale)
        return self

    def fade_copy(self, scale):
        ret = RGBW.from_rgb(self)
        ret.fade(scale)
        return ret

    # --------- Component Properties ---------

    @property
    def raw(self):
        return (self.r, self.g, self.b, self.w)

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

    @property
    def white(self):
        return self.w

    @white.setter
    def white(self, value):
        self.w = value
