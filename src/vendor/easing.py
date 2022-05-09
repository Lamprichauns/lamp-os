# https://github.com/semitable/easing-functions
# adaptation without the alpha/callable for more atomic use
import math

limit = (0, 1)

"""
Linear
"""
def linear(t):
    return t

"""
Quadratic easing functions
"""
def quad_ease_in(t):
    return t * t

def quad_ease_out(t):
    return -(t * (t - 2))

def quad_ease_in_out(t):
    if t < 0.5:
        return 2 * t * t
    return (-2 * t * t) + (4 * t) - 1

"""
Cubic easing functions
"""
def cubic_ease_in(t):
    return t * t * t

def cubic_ease_out(t):
    return (t - 1) * (t - 1) * (t - 1) + 1

def cubic_ease_in_out(t):
    if t < 0.5:
        return 4 * t * t * t
    p = 2 * t - 2
    return 0.5 * p * p * p + 1

"""
Quartic easing functions
"""
def quartic_ease_in(t):
    return t * t * t * t

def quartic_ease_out(t):
    return (t - 1) * (t - 1) * (t - 1) * (1 - t) + 1

def quartic_ease_in_out(t):
    if t < 0.5:
        return 8 * t * t * t * t
    p = t - 1
    return -8 * p * p * p * p + 1

"""
Quintic easing functions
"""
def quintic_ease_in(t):
    return t * t * t * t * t

def quintic_ease_out(t):
    return (t - 1) * (t - 1) * (t - 1) * (t - 1) * (t - 1) + 1

def quintic_ease_in_out(t):
    if t < 0.5:
        return 16 * t * t * t * t * t
    p = (2 * t) - 2
    return 0.5 * p * p * p * p * p + 1

"""
Sine easing functions
"""
def sine_ease_in(t):
    return math.sin((t - 1) * math.pi / 2) + 1

def sine_ease_out(t):
    return math.sin(t * math.pi / 2)

def sine_ease_in_out(t):
    return 0.5 * (1 - math.cos(t * math.pi))

"""
Circular easing functions
"""
def circular_ease_in(t):
    return 1 - math.sqrt(1 - (t * t))

def circular_ease_out(t):
    return math.sqrt((2 - t) * t)

def circular_ease_in_out(t):
    if t < 0.5:
        return 0.5 * (1 - math.sqrt(1 - 4 * (t * t)))
    return 0.5 * (math.sqrt(-((2 * t) - 3) * ((2 * t) - 1)) + 1)


"""
Exponential easing functions
"""
def exponential_ease_in(t):
    if t == 0:
        return 0
    return math.pow(2, 10 * (t - 1))

def exponential_ease_out(t):
    if t == 1:
        return 1
    return 1 - math.pow(2, -10 * t)

def exponential_ease_in_out(t):
    if t == 0 or t == 1:
        return t

    if t < 0.5:
        return 0.5 * math.pow(2, (20 * t) - 10)
    return -0.5 * math.pow(2, (-20 * t) + 10) + 1


"""
Elastic Easing Functions
"""
def elastic_ease_in(t):
    return math.sin(13 * math.pi / 2 * t) * math.pow(2, 10 * (t - 1))

def elastic_ease_out(t):
    return math.sin(-13 * math.pi / 2 * (t + 1)) * math.pow(2, -10 * t) + 1

def elastic_ease_in_out(t):
    if t < 0.5:
        return (
            0.5
            * math.sin(13 * math.pi / 2 * (2 * t))
            * math.pow(2, 10 * ((2 * t) - 1))
        )
    return 0.5 * (
        math.sin(-13 * math.pi / 2 * ((2 * t - 1) + 1))
        * math.pow(2, -10 * (2 * t - 1))
        + 2
    )

"""
Back Easing Functions
"""
def back_ease_in(t):
    return t * t * t - t * math.sin(t * math.pi)

def back_ease_out(t):
    p = 1 - t
    return 1 - (p * p * p - p * math.sin(p * math.pi))

def back_ease_in_out(t):
    if t < 0.5:
        p = 2 * t
        return 0.5 * (p * p * p - p * math.sin(p * math.pi))

    p = 1 - (2 * t - 1)

    return 0.5 * (1 - (p * p * p - p * math.sin(p * math.pi))) + 0.5


"""
Bounce Easing Functions
"""
def bounce_ease_in(t):
    return 1 - bounce_ease_out(1 - t)

def bounce_ease_out(t):
    if t < 4 / 11:
        return 121 * t * t / 16
    elif t < 8 / 11:
        return (363 / 40.0 * t * t) - (99 / 10.0 * t) + 17 / 5.0
    elif t < 9 / 10:
        return (4356 / 361.0 * t * t) - (35442 / 1805.0 * t) + 16061 / 1805.0
    return (54 / 5.0 * t * t) - (513 / 25.0 * t) + 268 / 25.0

def bounce_ease_in_out(t):
    if t < 0.5:
        return 0.5 * bounce_ease_in(t * 2)
    return 0.5 * bounce_ease_out(t * 2 - 1) + 0.5

"""
Easing function for use with float values over a given number of steps 
"""
def ease(start, end, duration, current_step, easing_function=quad_ease_in_out):
    t = limit[0] * (1 - current_step) + limit[1] * current_step
    t /= duration
    a = easing_function(t)
    return end * a + start * (1 - a)
