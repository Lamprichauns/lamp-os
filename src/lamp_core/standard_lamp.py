from components.led.led_strip_6812_rgbww import LedStrip6812RGBWW
from components.network.bluetooth import Bluetooth
from components.motion.motion_6050 import MotionMPU6050
from components.touch.touch import Touch
from behaviours.defaults import LampFadeIn
from lamp_core.lamp import Lamp

# Use standard lamp to startup a lamp that uses Lamprichaun hardware
class StandardLamp(Lamp):
    def __init__(self, name, base_color, shade_color):
        super().__init__(name)

        self.base = LedStrip6812RGBWW(base_color, pin=12, num_pixels=40)
        self.shade = LedStrip6812RGBWW(shade_color, pin=13, num_pixels=40)
        self.bluetooth = Bluetooth(name, base_color, shade_color)
        self.network = self.bluetooth.network
        self.bluetooth.enable()
        self.motion = MotionMPU6050(pin_sda=21, pin_scl=22)
        self.touch = Touch(pin=32)

        self.add_behaviour(LampFadeIn)
