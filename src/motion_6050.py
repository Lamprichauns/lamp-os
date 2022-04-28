from machine import I2C, Pin
from math import sqrt, pow, floor
import mpu6050

# Abstraction for lamp motion using the onboard MPU-6050 accelerometer
class LampMotionMPU6050:
    def __init__(self, pin_sda, pin_scl):
        self.i2c = I2C(sda=Pin(pin_sda), scl=Pin(pin_scl))
        self.accelerometer = mpu6050.accel(self.i2c)
        self.previous_sample = 0.0;

    # Get an unsigned movement intensity value over a monotonic sample period
    # @see https://www.egr.msu.edu/~mizhang/papers/feature-selection-activity-recognition-bodynets11.pdf
    def get_movement_intensity_value(self):
        value = self.accelerometer.get_values();
        norm = sqrt(pow(abs(value['AcZ']), 2) + pow(abs(value['AcY']),2) + pow(abs(value['AcX']),2));
        difference = abs(self.previous_sample - norm)
        self.previous_sample = norm

        return difference
