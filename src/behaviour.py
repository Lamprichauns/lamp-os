import neopixel, machine
import uasyncio as asyncio
from led_strip import LedStrip

class Behaviour(LedStrip):
    def __init__(self,lamp):
        self.lamp = lamp

    def __str__(self):
        return self.__class__.__name__
