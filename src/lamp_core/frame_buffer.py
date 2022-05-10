# Allow all behaviors to contribute their pixels to the scene
#  - self.buffer holds a list of RGBW tuples for the scene
#  - write it to the LED driver using flush
#  - color order will be handled by the driver at write time
#  - optionally apply a brightness contour to your lamp with the post process function
class FrameBuffer():
    def __init__(self, default_color, num_pixels, driver, post_process_function=None):
        self.post_process_function = post_process_function
        self.default_color = default_color
        self.num_pixels = num_pixels
        self.driver = driver
        self.previous_buffer = [default_color] * self.num_pixels
        self.buffer = [default_color] * self.num_pixels
        # In case your lamp needs to go back to its original state, store a
        # copy of the ideal start state. Behaviors can use this information
        # to return the lamp to a default state
        self.default_pixels = [default_color] * self.num_pixels

    # Set entire buffer to a new solid color
    def fill(self, color):
        self.buffer = [color] * self.num_pixels

    # Write the final scene to the driver as a list of int 4-tuples in RGBW order
    def flush(self):
        # fast sanity check before doing computation
        if self.previous_buffer == self.buffer:
            return

        self.previous_buffer = self.buffer.copy()

        if self.post_process_function is not None:
            self.post_process_function(self.buffer)

        try:
            self.driver.write([(int(r), int(g), int(b), int(w)) for r, g, b, w in self.buffer])
        except:
            print('Memory overage while painting')
