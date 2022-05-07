from vendor.easing import QuadEaseInOut

# a basic fade over a number of steps
def fade(start, end, current_step, steps, easing_function=QuadEaseInOut):
    return (
        easing_function(start[0], end = end[0], duration = steps)(current_step),
        easing_function(start[1], end = end[1], duration = steps)(current_step),
        easing_function(start[2], end = end[2], duration = steps)(current_step),
        easing_function(start[3], end = end[3], duration = steps)(current_step)
    )

# A pingpong effect for your fades start color -> middle color -> end color
def pingpong_fade(start, middle, end, current_step, steps, easing_function_in=QuadEaseInOut, easing_function_out=QuadEaseInOut):
    loop_point = int(steps/2)
    if current_step < loop_point:
        return fade(start, middle, current_step, loop_point, easing_function_in)

    return fade(middle, end, current_step-loop_point, loop_point, easing_function_out)
