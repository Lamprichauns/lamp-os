# Abstraction for ambient temperature using the MPU-6050 IMU
class TemperatureMPU6050:
    def __init__(self, accelerometer):
        self.accelerometer = accelerometer

    # Get an ambient temperature reading
    def get_temperature_value(self):
        value = self.accelerometer.get_values()
        return value["Tmp"]
