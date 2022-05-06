from vendor.easing import QuadEaseInOut

def fade(start, end, current_step, steps, easing_function=QuadEaseInOut):
    return (
        easing_function(start[0], end = end[0], duration = steps)(current_step),
        easing_function(start[1], end = end[1], duration = steps)(current_step),
        easing_function(start[2], end = end[2], duration = steps)(current_step),
        easing_function(start[3], end = end[3], duration = steps)(current_step)
    )
