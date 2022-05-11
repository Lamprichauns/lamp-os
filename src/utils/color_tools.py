from math import ceil

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
