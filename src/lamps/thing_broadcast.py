from machine import Pin
from ..lamp_core.base_lamp import BaseLamp
from ..network.coding import Codes
from ..colors import *

class ThingBroadcast(BaseLamp):
    '''
    Simple Lamp that just uses the ESP32 Thing to broadcast a message
    when the button is pressed.
    '''

    LED_PIN = 5
    BUTTON_PIN = 0

    def __init__(self, network) -> None:
        super().__init__(network, 0xff0000, 0xffffff)

        self.led = Pin(self.LED_PIN, Pin.OUT)
        self.button_pin = Pin(self.BUTTON_PIN, Pin.IN, Pin.PULL_UP)

        self.led.value(0)

    async def update(self):
        if self.button_pin.value() == 0:
            self.led.value(1)
            self.network.send_broadcast(Codes.BASE_OVERRIDE, (16, 0, 192, 0))
            self.network.send_broadcast(Codes.SHADE_OVERRIDE, (0, 255, 0, 0))
        else:
            self.led.value(0)

    async def new_lamp_appeared(self, new_lamp):
        print(f'New Lamp: {new_lamp}')

    async def lamp_changed(self, lamp):
        pass

    async def lamps_departed(self, lamps):
        print(f'Lamps departed: {lamps}')

    async def message_observed(self, message):
        print(f'Message: {message.code} - {message.payload}')

    async def message_stopped(self, code):
        pass
