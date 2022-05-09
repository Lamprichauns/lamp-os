from vendor.easing import linear
from utils.fade import fade

# Create a gradient between one color and another. Input colors should be 4-tuples of RGBW
def create_gradient(color_start, color_end, steps, easing_function=linear):
    color_list = [(0, 0, 0, 0)] * steps

    for i in range(steps):
        color_list[i] = fade(color_start, color_end, i, steps, easing_function = easing_function)

    return color_list
