from ..lamp_core.base_lamp import BaseLamp
from ..network.coding import Codes
from ..colors import *

class SimpleLamp(BaseLamp):
    BASE_COLOR = RGBW(0, 255, 0, 0)
    SHADE_COLOR = RGBW(255, 197, 143, 255)

    SHADE_PIN = 13
    BASE_PIN = 12

    def __init__(self, network) -> None:
        super().__init__(network, self.BASE_COLOR, self.SHADE_COLOR)

        # Tell the networking layer to listen for other lamp events
        network.add_observer(self)

        self.shade_color = self.SHADE_COLOR

        # Create LEDs for the SHADE
        self.shade_pixels = RGBWList(40)
        self.shade_driver = NeoPixel(self.SHADE_PIN, self.shade_pixels, ColorOrder.GRBW)

        # Create LEDs for the BASE
        self.base_pixels = RGBWList(40)
        self.base_driver = NeoPixel(self.BASE_PIN, self.base_pixels, ColorOrder.GRBW)

        self.color_step = 0

    # Called to update the frame
    async def update(self):
        self.shade_pixels.fill(self.shade_color)

        # The base color will hue cycle
        self.color_step %= 0xff
        base_color = RGBW.from_hsv(HSV(self.color_step))
        self.base_pixels.fill(base_color)
        self.color_step += 3

        # Write the lighting data to the leds
        self.shade_driver.write()
        self.base_driver.write()

    # Called any time we spot a new lamp in the wild
    async def new_lamp_appeared(self, new_lamp):
        print(f'New Lamp: {new_lamp}')

    # Called everytime anything changes about the lamp, including
    # rssi. It's a pretty noisey function
    async def lamp_changed(self, lamp):
        pass

    # Called everytime a specific attribute changes, so you can
    # respond to just a BASE_COLOR change or something like that
    async def lamp_attribute_changed(self, lamp, attribute):
        print(f"Attribute: {lamp.name} - {attribute.code}")

    # List which lamps no longer are around.
    async def lamps_departed(self, lamps):
        print(f'Lamps departed: {lamps}')
        pass

    # Called when a broadcast message is recieved.
    async def message_observed(self, message):
        if message.code == Codes.SHADE_OVERRIDE:
            print("Switching to SHADE_OVERRIDE color")
            self.shade_color = RGBW.from_(message.payload)

    # Called when a broadcast message has expired and not longer should
    # be observed
    async def message_stopped(self, code):
        if code == Codes.SHADE_OVERRIDE:
            print("Stopping SHADE_OVERRIDE color")
            self.shade_color = self.SHADE_COLOR
