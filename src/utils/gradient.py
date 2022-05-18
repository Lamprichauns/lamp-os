import micropython
from utils.easing import linear
from utils.fade import fade

# Create a gradient between one color and another. Input colors should be 4-tuples of RGBW
@micropython.native
def create_gradient(color_start, color_end, steps, easing_function=None):
    color_list = [(0, 0, 0, 0)] * steps

    for i in range(steps):
        color_list[i] = fade(color_start, color_end, steps, i, easing_function or linear)

    return color_list
