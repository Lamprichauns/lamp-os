from vendor.easing import LinearInOut

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
