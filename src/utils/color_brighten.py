from math import ceil

def level(val):
    return 255 if val > 255 else val

# Brighten an rgbw value by a percentage
def brighten(color, percentage):
    p = percentage/100 + 2

    return (
        level(ceil(color[0]*p)),
        level(ceil(color[1]*p)),
        level(ceil(color[2]*p)),
        level(ceil(color[3]*p))
    )
