import machine
from time import sleep_ms

# Capacitive touch stuff to make the lamp base into touch sensors
class LampTouch:
    # Initialize for a given pin
    def __init__(self, pin): 
        self.pin = machine.TouchPad(machine.Pin(pin))
        calibration = []

        self.avg = self.read_averaged(25)

        print("Touch calibrated: %s" % (self.avg))
    
    # Return the value of the sensor
    def value(self):
        return self.pin.read()


    def read_averaged(self,count):
        values = []        
        for x in range(count):
            values.append(self.pin.read())
            sleep_ms(1)

        return sum(values) // len(values)

    # Are we being touched? 
    def is_touched(self):
        current = self.read_averaged(5)
        diff = int((self.value() / self.avg) * 100)

        touched = (40 <= diff <= 95)

        print("Touch reading: %s" % (current))
        return  touched
