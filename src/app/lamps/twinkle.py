import random
from ..lamp_core.base_lamp import BaseLamp
from ..network.coding import Codes
from ..colors import *

class Twinkle(BaseLamp):
    SHADE_COLOR = RGBW(255, 197, 143, 255)

    def __init__(self, network) -> None:
        network.add_observer(self)

        self.twinkle = TwinkleAnimation()
        self.shade = ShadeAnimation(self.SHADE_COLOR)

        super().__init__(network, self.twinkle.color, self.shade.color)

    async def update(self):
        self.twinkle.should_rainbow = len(self.network.model.visible_lamps) > 1

        self.twinkle.update()
        self.shade.update()

    async def new_lamp_appeared(self, new_lamp):
        print(f'New Lamp: {new_lamp}')

    async def lamp_changed(self, lamp):
        pass

    async def lamp_attribute_changed(self, lamp, attribute):
        print(f"Attribute: {lamp.name} - {attribute.code}")

    async def lamps_departed(self, lamps):
        pass

    async def message_observed(self, message):
        if message.code == Codes.BASE_OVERRIDE:
            print("Switching to BASE_OVERRIDE color")
            self.twinkle.override_color = RGB.from_tuple(message.payload)

    async def message_stopped(self, code):
        if code == Codes.BASE_OVERRIDE:
            print("Stopping BASE_OVERRIDE color")
            self.twinkle.override_color = None

class TwinkleAnimation:
    PIN = 12
    TOTAL_PIXELS = 100

    COLORS = [RGB(0, 150, 50)]

    def __init__(self):
        self.pixels = RGBList(self.TOTAL_PIXELS)
        self.driver = NeoPixel(self.PIN, self.pixels)

        self.override_color = None

        self.color_shift = 0
        self.should_rainbow = False

        self.fade_rate = 240
        self.color_cycle = 0

        self.color = self.COLORS[0]

    def get_new_color(self):
        if self.override_color:
            return self.override_color
        
        if self.should_rainbow:
            self.color_shift += 3
            self.color_shift %= 255
            return RGB.from_hsv(HSV(self.color_shift))

        return self.color

    def update(self):
        self.pixels.fade(self.fade_rate)

        len = self.pixels.size
        max_rand = len >> (4 if self.fade_rate < 220 else 5)
        new_points = random.randint(0, max_rand)

        for _ in range(new_points):
            index = random.randint(0, len - 1)
            if self.should_rainbow:
                self.pixels[index] = self.get_new_color()
            else: 
                self.pixels[index] += self.get_new_color()

        self.driver.write()

class ShadeAnimation:
    PIN = 13
    TOTAL_PIXELS = 30

    def __init__(self, color):
        self.pixels = RGBWList(self.TOTAL_PIXELS)
        self.driver = NeoPixel(self.PIN, self.pixels, ColorOrder.GRBW)
        self.color = color

        self.pixels.fill(self.color)
        self.driver.write()

    def update(self):
        pass
