import machine
from time import sleep_ms

# Capacitive touch stuff to make the lamp base into touch sensors
class LampTouch:
    # Initialize for a given pin
    def __init__(self, pin): 
        self.pin = machine.TouchPad(machine.Pin(pin))
        calibration = []

        for x in range(12):
            calibration.append(self.pin.read())
            sleep_ms(1)

        self.avg = sum(calibration) // len(calibration)
        print("Touch calibrated: %s" % (self.avg))
    
    # Return the value of the sensor
    def value(self):
        return self.pin.read()

    # Are we being touched? Should be between 40-90% drop in reading
    def is_touched(self):
        current = self.value()
        diff = int((self.value() / self.avg) * 100)
        #print(diff)

        return  (40 <= diff <= 90)
