import neopixel, machine
import uasyncio as asyncio
from led_strip import LedStrip

class Behaviour(LedStrip):
    def __str__(self):
        self.__class__.__name__
        # TODO: Fix this. It returns "None", so i'm missing something simple