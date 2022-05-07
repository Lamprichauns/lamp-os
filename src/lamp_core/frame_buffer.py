#  Allow all behaviors to contribute their pixels to the scene
# and then flush it to the LED driver
class FrameBuffer():
    def __init__(self, default_color, num_pixels, driver):
        self.default_color = default_color
        self.num_pixels = num_pixels
        self.driver = driver
        self.buffer = [default_color] * self.num_pixels

    # Set entire buffer to a new solid color
    def fill(self, color):
        self.buffer = [color] * self.num_pixels

    # Write the final scene to the LED strip
    def flush(self):
        for p in range(self.num_pixels):
            self.driver[p] = (
                int(self.buffer[p][0]),
                int(self.buffer[p][1]),
                int(self.buffer[p][2]),
                int(self.buffer[p][3])
            )

        self.driver.write()
