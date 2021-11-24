from led_strip import LedStrip
import uasyncio as asyncio

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
    
    # Wake up the lamp and kick off the main loop
    def wake(self):
        self.shade.off()
        self.base.off()
 
        asyncio.create_task(self.shade.reset())
        asyncio.create_task(self.base.reset())

        asyncio.run(self.main())
    
    # The main loop
    async def main(self):
        print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep_ms(50)
