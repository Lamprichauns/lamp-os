from led_strip import LedStrip
import uasyncio

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

        uasyncio.run(self.wake())
 
    async def wake(self):
        # This is currently not working; it waits until the first coroutine is done before moving on
        shade_reset = uasyncio.create_task(self.shade.reset())
        base_reset = uasyncio.create_task(self.base.reset())
        await shade_reset, base_reset

        print("%s is awake!" % (self.name))

        while True:
            await uasyncio.sleep_ms(50)
