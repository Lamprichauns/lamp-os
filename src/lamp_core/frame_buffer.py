# Allow all behaviors to contribute their pixels to the scene
#  - self.buffer holds a list of RGBW tuples for the scene
#  - write it to the LED driver using flush
#  - color order will be handled by the driver at write time
class FrameBuffer():
    def __init__(self, default_color, num_pixels, driver):
        self.default_color = default_color
        self.num_pixels = num_pixels
        self.driver = driver
        self.previous_buffer = [default_color] * self.num_pixels
        self.buffer = [default_color] * self.num_pixels

    # Set entire buffer to a new solid color
    def fill(self, color):
        self.buffer = [color] * self.num_pixels

    # Write the final scene to the LED strip if necessary
    def flush(self):
        if self.buffer == self.previous_buffer:
            return

        self.previous_buffer = self.buffer.copy()

        self.driver.write(self.buffer)
