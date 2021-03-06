# pylint: disable=no-name-in-module
from time import sleep_ms
import machine

# Capacitive touch stuff to make the lamp base into touch sensors
class Touch:
    # Initialize for a given pin
    def __init__(self, pin):
        self.pin = machine.TouchPad(machine.Pin(pin))
        self.averages = self.read_values(20)

        self.touched = False

    def update_averages(self):
        value = self.value()
        if value != 0:
            self.averages.insert(0, value)
            self.averages.pop()

    def average(self):
        if len(self.averages) == 0:
            return 0

        return sum(self.averages) // len(self.averages)

    # Return the value of the sensor
    def value(self):
        try:
            return self.pin.read()
        except ValueError:
            return self.average()

    def read_values(self, count=15):
        values = []

        for _ in range(count):
            try:
                val = self.pin.read()
                values.append(val)
            except ValueError:
                pass

            sleep_ms(1)

        return values

    def read_averaged(self,count=15):
        values = self.read_values(count)

        if len(values) == 0:
            return 0

        return sum(values) // len(values)

    # Are we being touched?
    def is_touched(self):
        read = self.read_averaged(10)
        average = (read / self.average()) * 100

        #print("%s : %s (%s)" % (read, self.average(),  average))
        if average <= 90:
            return True

        self.update_averages()
        return False
