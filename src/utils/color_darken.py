from math import ceil

def level(val):
    return 0 if val < 1 else val

# Reduce the rgbw intensity of a value by a percentage
def darken (color, percentage):
    p = 1-(percentage/100)

    return (
        level(ceil(color[0]*p)),
        level(ceil(color[1]*p)),
        level(ceil(color[2]*p)),
        level(ceil(color[3]*p))
    )
