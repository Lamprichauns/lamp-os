from math import ceil
from vendor.easing import LinearInOut

def level_brighten(val):
    return 255 if val > 255 else val

def level_darken(val):
    return 0 if val < 1 else val

# Brighten an rgbw value by a percentage
def brighten(color, percentage):
    p = percentage/100 + 2

    return (
        level_brighten(ceil(color[0]*p)),
        level_brighten(ceil(color[1]*p)),
        level_brighten(ceil(color[2]*p)),
        level_brighten(ceil(color[3]*p))
    )

# Reduce the rgbw intensity of a value by a percentage
def darken (color, percentage):
    p = 1-(percentage/100)

    return (
        level_darken(ceil(color[0]*p)),
        level_darken(ceil(color[1]*p)),
        level_darken(ceil(color[2]*p)),
        level_darken(ceil(color[3]*p))
    )

# Create a gradient between one color and another. Input colors should be 4-tuples of RGBW
def create_gradient(color_start, color_end, steps, easing_function=LinearInOut):
    color_list = [(0, 0, 0, 0)] * steps
    for i in range(steps):
        color_list[i] = (
            int(easing_function(start = color_start[0], end = color_end[0], duration = steps)(i)),
            int(easing_function(start = color_start[1], end = color_end[1], duration = steps)(i)),
            int(easing_function(start = color_start[2], end = color_end[2], duration = steps)(i)),
            int(easing_function(start = color_start[3], end = color_end[3], duration = steps)(i)),
        )

    return color_list
