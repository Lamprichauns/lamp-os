from machine import I2C, Pin
import mpu6050

# Abstraction for lamp motion using the onboard MPU-6050 accelerometer
# To allow for different mounting options for your lamp pcb,
# gravity_axis will inform the software what axis gravity is acting on
class LampMotionMPU6050:
    def __init__(self, pin_sda, pin_scl, gravity_axis):
        self.gyro_plane = gyro_plane
        i2c = I2C(sda=Pin(pin_sda), scl=Pin(pin_scl))
        accelerometer = mpu6050.accel(i2c)

    def getValues():
        return accelerometer.get_values()
