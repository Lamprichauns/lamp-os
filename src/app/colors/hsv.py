'''
Tools for handle HSV style color representations.
''' 

_HSV_SECTION_3 = 0x40

class HSV:
    '''Simple HSV representation, mostly borrowed from FastLED'''
    HUE_RED = 0,
    HUE_ORANGE = 32,
    HUE_YELLOW = 64,
    HUE_GREEN = 96,
    HUE_AQUA = 128,
    HUE_BLUE = 160,
    HUE_PURPLE = 192,
    HUE_PINK = 224

    def __init__(self, h=0, s=255, v=255):
        self.h = h
        self.s = s
        self.v = v

    # Shamelessly stolen from FastLED: 
    # https://github.com/FastLED/FastLED/blob/master/src/hsv2rgb.cpp
    # Lots of good stuff in that file, but this was the simplest
    def to_raw_rgb(self):
        value = self.val
        saturation = self.sat

        invsat = 255 - saturation
        brightness_floor = (value * invsat) >> 8

        color_amplitude = value - brightness_floor

        section = int(self.hue / _HSV_SECTION_3)
        offset = self.hue % _HSV_SECTION_3

        rampup = offset
        rampdown = (_HSV_SECTION_3 - 1) - offset

        rampup_amp_adj   = (rampup   * color_amplitude) >> 6
        rampdown_amp_adj = (rampdown * color_amplitude) >> 6

        rampup_adj_with_floor   = rampup_amp_adj   + brightness_floor
        rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor

        if section != 0:
            if section == 1:
                return (brightness_floor, rampdown_adj_with_floor, rampup_adj_with_floor)
            else:
                return (rampup_adj_with_floor, brightness_floor, rampdown_adj_with_floor)
        else:
            return (rampdown_adj_with_floor, rampup_adj_with_floor, brightness_floor)

    @property
    def hue(self):
        return self.h

    @hue.setter
    def hue(self, value):
        self.h = value

    @property
    def sat(self):
        return self.s

    @sat.setter
    def sat(self, value):
        self.s = value

    @property
    def val(self):
        return self.v

    @val.setter
    def val(self, value):
        self.v = value
