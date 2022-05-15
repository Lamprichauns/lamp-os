# https://github.com/semitable/easing-functions
# adaptation without the alpha/callable for more atomic use
import micropython

limit = (0, 1)

@micropython.native
def linear(t):
    return t

@micropython.native
def quad_ease_in_out(t):
    if t < 0.5:
        return 2 * t * t
    return (-2 * t * t) + (4 * t) - 1

@micropython.native
def ease(start, end, duration, current_step, easing_function=quad_ease_in_out):
    a = easing_function(((limit[0] * (1 - current_step) + limit[1] * current_step) / duration))
    return end * a + start * (1 - a)

@micropython.native
def pingpong_ease(start, middle, end, steps, current_step, easing_function=quad_ease_in_out):
    loop_point = int(steps/2)
    if current_step < loop_point:
        return ease(start, middle, steps/2, current_step)

    return ease(middle, end, steps/2, current_step % loop_point)
