from components.led.led_strip_2812_rgb import LedStrip2812RGB
from components.network.bluetooth import Bluetooth
from components.motion.motion_6050 import MotionMPU6050
from components.touch.touch import Touch
from behaviours.defaults import LampFadeIn, StartNetworking
from lamp_core.lamp import Lamp

# Use standard lamp to startup a lamp that uses Lamprichaun hardware
class StandardLamp(Lamp):
    def __init__(self, name, base_color, shade_color):
        super().__init__(name)

        self.base = LedStrip2812RGB(self, base_color, pin=12, num_pixels=40)
        self.shade = LedStrip2812RGB(self, shade_color, pin=13, num_pixels=40)
        self.bluetooth = Bluetooth(name, base_color, shade_color)
        self.network = self.bluetooth.network
        self.bluetooth.enable()
        self.motion = MotionMPU6050(pin_sda=21, pin_scl=22)
        self.touch = Touch(pin=32)

        self.add_behaviour(LampFadeIn)

    # Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
    # instead of each individual color)
    def hex_to_rgbw(self, value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)
