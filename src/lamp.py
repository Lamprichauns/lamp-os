from led_strip import LedStrip
import uasyncio as asyncio
from touch import LampTouch
from bt import LampBluetooth
import re

# Default config for pins and pixels
default_config = {   
    "base":  { "pin": 12, "pixels": 40 },
    "shade": { "pin": 13, "pixels": 40 }, 
    "touch": { "pin": 32 }
}

# We love lamp.
class Lamp:
    def __init__(self, name, base_color, shade_color, config = default_config):
        if not re.match('^[a-z]+$', name): raise NameError('Name must be lowercase alpha')

        self.lock = asyncio.Lock()
        self.name = name
        self.behaviours = []
        
        self.touch = LampTouch(config["touch"]["pin"])        
        self.shade = LedStrip(self, shade_color, config['shade']['pin'], config['shade']['pixels'])
        self.base = LedStrip(self, base_color, config['base']['pin'], config['base']['pixels'])

        self.bt = LampBluetooth(name, base_color, shade_color)
        self.network = self.bt.network

    # Add a behaviour 
    def add_behaviour(self, behaviour_class):
        b = behaviour_class(self)

        if any(isinstance(x, behaviour_class) for x in self.behaviours):
            print("Behaviour already added (%s)" % (b))
        else: 
            self.behaviours.append(b)
            print("Behaviour added: %s" % (b))

    # Return a behaviour instance 
    def behaviour(self, behaviour_class): 
        return next(x for x in self.behaviours if isinstance(x, behaviour_class))
    
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
        self.base.off()
        self.shade.off()

        shade_fade = asyncio.create_task(self.shade.async_fade(self.shade.default_pixels,40))
        base_fade = asyncio.create_task(self.base.async_fade(self.base.default_pixels,40))

        await asyncio.gather(shade_fade,base_fade)

        for behaviour in self.behaviours:
            print("Enabling Behaviour: %s" % (behaviour))
            asyncio.create_task(behaviour.run())
        
        self.bt.enable()

        print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep_ms(1)
            await self.network.monitor()