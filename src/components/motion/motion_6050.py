from math import sqrt, pow as power
from machine import SoftI2C, Pin
from vendor.mpu6050 import accel

# Abstraction for lamp motion using an MPU-6050 accelerometer
class MotionMPU6050:
    def __init__(self, pin_sda, pin_scl):
        self.i2c = SoftI2C(sda=Pin(pin_sda), scl=Pin(pin_scl), freq=400000)
        self.accelerometer = accel(self.i2c)
        self.previous_sample = 0.0

    # Get an unsigned movement intensity value over a monotonic sample period
    # @see https://www.egr.msu.edu/~mizhang/papers/feature-selection-activity-recognition-bodynets11.pdf
    def get_movement_intensity_value(self):
        value = self.accelerometer.get_values()
        norm = sqrt(power(abs(value['AcZ']), 2) + power(abs(value['AcY']),2) + power(abs(value['AcX']),2))
        difference = 0 if(self.previous_sample < 1) else abs(self.previous_sample - norm)
        self.previous_sample = norm

        return difference
