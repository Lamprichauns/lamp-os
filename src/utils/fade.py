import micropython
from utils.easing import ease, quad_ease_in_out

# a basic fade over a number of steps
@micropython.native
def fade(start, end, steps, current_step, easing_function=None):
    lst = easing_function or quad_ease_in_out
    return (
        end[0] if start[0] == end[0] else ease(start = start[0], end = end[0], duration = steps, current_step = current_step, easing_function = lst),
        end[1] if start[1] == end[1] else ease(start = start[1], end = end[1], duration = steps, current_step = current_step, easing_function = lst),
        end[2] if start[2] == end[2] else ease(start = start[2], end = end[2], duration = steps, current_step = current_step, easing_function = lst),
        end[3] if start[3] == end[3] else ease(start = start[3], end = end[3], duration = steps, current_step = current_step, easing_function = lst)
    )

# A pingpong effect for your fades start color -> middle color -> end color
@micropython.native
def pingpong_fade(start, middle, end, steps, current_step, easing_function_in = None, easing_function_out = None):
    loop_point = int(steps//2)
    if current_step < loop_point:
        return fade(start, middle, loop_point, current_step, easing_function_in or quad_ease_in_out)

    return fade(middle, end, loop_point, current_step-loop_point, easing_function_out or quad_ease_in_out)
