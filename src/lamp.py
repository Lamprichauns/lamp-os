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
        self.behaviours = []
        self.shade = LedStrip(shade_color, pixel_config['shade']['pin'], pixel_config['shade']['pixels'])
        self.base = LedStrip(base_color, pixel_config['base']['pin'], pixel_config['base']['pixels'])
    
    def add_behaviour(self,behaviour_class):
        b = behaviour_class(self)
        print(b)
        asyncio.create_task(b.run())
        
        self.behaviours.append(b) 
        print("Behaviour added: %s" % (b))

    # Reset the lamp to it's default colors
    def reset(self):
        self.shade.reset()
        self.base.reset()

    # Turn the lamp off
    def off(self):
        self.shade.off()
        self.base.off()

    # Wake up the lamp and kick off the main loop
    def wake(self):
        asyncio.run(self.main())

    # The main loop
    async def main(self):
        self.off()

        asyncio.create_task(self.shade.until_faded_to(self.shade.color,100))
        asyncio.create_task(self.base.until_faded_to(self.base.color,100))

        print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep_ms(50)
