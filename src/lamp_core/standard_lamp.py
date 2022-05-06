from components.led.led_strip_6812_rgbww import LedStrip6812RGBWW
from components.network.bluetooth import Bluetooth
from components.motion.motion_6050 import MotionMPU6050
from components.touch.touch import Touch
from behaviours.defaults import LampFadeIn
from lamp_core.lamp import Lamp


default_config = {
    "base":  { "pin": 12, "pixels": 40 },
    "shade": { "pin": 13, "pixels": 40 },
    "touch": { "pin": 32 }
}


# Use standard lamp to startup a lamp that uses Lamprichaun hardware
class StandardLamp(Lamp):
    def __init__(self, name, base_color, shade_color, config_opts = {}):
        config = default_config.copy()
        config.update(config_opts)

        super().__init__(name)

        self.base = NeoPixelStrip(base_color, pin=config["base"]["pin"], num_pixels=config["base"]["pixels"])
        self.shade = NeoPixelStrip(shade_color, pin=config["shade"]["pin"], num_pixels=config["base"]["pixels"])
        self.bluetooth = Bluetooth(name, base_color, shade_color)
        self.network = self.bluetooth.network
        self.bluetooth.enable()
        #self.motion = MotionMPU6050(pin_sda=21, pin_scl=22)
        self.touch = Touch(pin=config["touch"]["pin"])

        self.add_behaviour(LampFadeIn)
