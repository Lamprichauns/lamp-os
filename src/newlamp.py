#import network, re, uasyncio
#from time import sleep
#from machine import Timer
#from simple_lamp import SimpleLamp
import neopixel, machine

machine.freq(240000000)

base_led_config     = { "pin": 12, "pixels": 5 }
shade_led_config    = { "pin": 13, "pixels": 5 }


import binascii, hashlib

class LightControlMixin: 
    # Reset to the configured color
    def reset(self): 
        self.pixels.fill(self.color)
        self.pixels.write()

    # Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
    # instead of each individual color)
    def hex_to_rgb(self, value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)

class Lamp:
    def __init__(self, name, base_color, shade_color):
        self.name = name
        self.shade = Shade(shade_color)
        self.base = Base(base_color)
        self.reset()

    def reset(self):
        self.shade.reset()
        self.base.reset()

class Base(LightControlMixin): 
    def __init__(self, color):
        self.color = self.hex_to_rgb(color) 
        self.num_pixels = base_led_config["pixels"]
        self.pixels = neopixel.NeoPixel(machine.Pin(base_led_config["pin"]), self.num_pixels, bpp=4, timing=1)
    
class Shade(LightControlMixin): 
    def __init__(self,color):
        self.color = self.hex_to_rgb(color) 
        self.num_pixels = shade_led_config["pixels"]
        self.pixels = neopixel.NeoPixel(machine.Pin(shade_led_config["pin"]), self.num_pixels, bpp=4, timing=1)
    

lamp = Lamp("gramp", "#ffffff", "#00ff00")
