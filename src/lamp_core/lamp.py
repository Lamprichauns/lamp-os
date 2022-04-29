from .base_lamp import BaseLamp
import uasyncio as asyncio
import re

# We love lamp.
class Lamp():
    def __init__(self, network, name, base_color, shade_color, config = default_config):
        super().__init__(network, base_color, shade_color)

        if not re.match('^[a-z]+$', name): raise NameError('Name must be lowercase alpha')

        self.lock = asyncio.Lock()
        self.name = name
        self.behaviours = []

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

    # Restart all behaviours
    async def reset(self):
        for behaviour in self.behaviours:
            print("Resetting Behaviour: %s" % (behaviour))
            asyncio.create_task(behaviour.reset())

    # Stop all lamp behaviors
    async def off(self):
        for behaviour in self.behaviours:
            print("Stopping Behaviour: %s" % (behaviour))
            asyncio.create_task(behaviour.stop())

    # Start a behaviour
    async def start(self):
        for behaviour in self.behaviours:
            print("Enabling Behaviour: %s" % (behaviour))
            asyncio.create_task(behaviour.run())
