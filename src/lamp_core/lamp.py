import re
import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour, DrawBehaviour, BackgroundBehavior

# We love lamp.
#
# Please use StandardLamp instead of using this class directly if you're using an
# ESP32 board with neopixels
# Check the examples in lamps for usage
class Lamp():
    def __init__(self, name):
        if not re.match('^[a-z]+$', name):
            raise NameError('Name must be lowercase alpha')
        self.name = name
        self.debug = False
        self.home_mode = False
        self.behaviours = []

    # Return a behaviour instance
    def behaviour(self, behaviour_class):
        return next(x for x in self.behaviours if isinstance(x, behaviour_class))

    # Add a behaviour
    def add_behaviour(self, behaviour_class):
        if self.home_mode is True and behaviour_class.use_in_home_mode is False:
            print("Behaviour rejected in home mode: %s" % (behaviour_class))
        else:
            self.behaviours.append(behaviour_class)
            print("Behaviour added: %s" % (behaviour_class))

    # Called once all components and behaviours added to begin all async tasks
    def wake(self):
        asyncio.run(self.start())

    # Start all behaviours
    async def start(self):
        for behaviour in self.behaviours:
            if isinstance(behaviour, AnimatedBehaviour):
                if self.debug is True:
                    print("Enabling Animation: %s" % (behaviour))
                asyncio.create_task(behaviour.run())

            if isinstance(behaviour, BackgroundBehavior):
                if self.debug is True:
                    print("Enabling Background Task: %s" % (behaviour))
                asyncio.create_task(behaviour.run())

        draw = DrawBehaviour(self)
        asyncio.create_task(draw.run())

        if self.debug is True:
            print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep(0)
