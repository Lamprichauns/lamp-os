def level_brighten(val):
    return 255 if val > 254 else val

def level_darken(val):
    return 0 if val < 1 else val

# Brighten an rgbw value by a percentage
def brighten(color, percentage):
    p = ((percentage)/100)+1

    return (
        level_brighten(color[0]*p),
        level_brighten(color[1]*p),
        level_brighten(color[2]*p),
        level_brighten(color[3]*p)
    )

# Reduce the rgbw intensity of a value by a percentage
def darken (color, percentage):
    p = 1-(percentage/100)

    return (
        level_darken(color[0]*p),
        level_darken(color[1]*p),
        level_darken(color[2]*p),
        level_darken(color[3]*p)
    )
