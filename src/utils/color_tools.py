import micropython

@micropython.native
def level_brighten(val):
    return 255 if val > 254 else val

@micropython.native
def level_darken(val):
    return 0 if val < 1 else val

# Brighten an rgbw value by a percentage
@micropython.native
def brighten(color, percentage):
    p = ((percentage*100)//100)+100

    return (
        level_brighten(color[0]*p//100),
        level_brighten(color[1]*p//100),
        level_brighten(color[2]*p//100),
        level_brighten(color[3]*p//100)
    )

# Reduce the rgbw intensity of a value by a percentage
@micropython.native
def darken (color, percentage):
    p = 100-((percentage*100)//100)

    return (
        level_darken(color[0]*p//100),
        level_darken(color[1]*p//100),
        level_darken(color[2]*p//100),
        level_darken(color[3]*p//100)
    )
