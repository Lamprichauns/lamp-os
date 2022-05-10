# Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
# instead of each individual color)
def hex_to_rgbw(value):
    value = value.lstrip('#')
    rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))

    return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)
