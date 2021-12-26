import machine
from time import sleep_ms
import uasyncio as asyncio

# Capacitive touch stuff to make the lamp base into touch sensors
class LampTouch:
    # Initialize for a given pin
    def __init__(self, pin): 
        self.pin = machine.TouchPad(machine.Pin(pin))
        self.avg = self.read_averaged(50)

        self.touched = False
 
    # Return the value of the sensor
    def value(self):
        try:
            return self.pin.read()
        except ValueError:
            return self.avg

    def read_averaged(self,count=15):
        values = []
        
        for x in range(count):
            try: 
                val = self.pin.read()
                values.append(val)
            except ValueError: 
                pass

            sleep_ms(1)

        return sum(values) // len(values)

    # Are we being touched? 
    def is_touched(self):
        read = self.read_averaged(50)
        average = (read / self.avg) * 100

        if average <= 92:
            return True
        else: 
            self.avg = (read + self.avg) / 2
            return False
