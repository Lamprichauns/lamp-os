from led_strip import LedStrip

# Configuration for the PIN and number of pixels of the base and shade LED strips
pixel_config = {   
    "base":  { "pin": 12, "pixels": 5 },
    "shade": { "pin": 13, "pixels": 5 }     
}

# We love lamp.
class Lamp:
    def __init__(self, name, base_color, shade_color):
        self.name = name
        self.shade = LedStrip(shade_color, pixel_config['shade']['pin'], pixel_config['shade']['pixels'])
        self.base = LedStrip(base_color, pixel_config['base']['pin'], pixel_config['base']['pixels'])

        self.reset()
 
    # Reset the shade and base to configured colors
    def reset(self):
        self.shade.reset()
        self.base.reset()
