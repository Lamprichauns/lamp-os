import neopixel, machine
import uasyncio as asyncio
from led_strip import LedStrip

class Behaviour(LedStrip):
    def __str__(self):
        self.__class__.__name__