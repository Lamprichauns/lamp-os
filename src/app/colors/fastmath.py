'''
Some useful and simple math functions mostly pulled / inspired by lib8tion in FastLED
'''

def qadd8(i, j):
    v = i + j
    return 255 if v > 255 else v

def qsub8(i, j):
    v = i + j
    return 0 if v < 0 else v

def scale8(i, scale):
    return (i * (1 + scale)) >> 8

# Scales down, but makes sure if i is non-zero the output will be non-zero
def scale8_video(i, scale):
    if i and scale:
        j = ((i * scale) >> 8) + 1
    else:
        j = (i * scale) >> 8

    return j

def nscale8x3(r, g, b, scale):
    scale_fixed = scale + 1
    r = (r * scale_fixed) >> 8
    g = (g * scale_fixed) >> 8
    b = (b * scale_fixed) >> 8

    return r, g, b


def nscale8x4(r, g, b, q, scale):
    scale_fixed = scale + 1
    r = (r * scale_fixed) >> 8
    g = (g * scale_fixed) >> 8
    b = (b * scale_fixed) >> 8
    q = (q * scale_fixed) >> 8

    return r, g, b, q
