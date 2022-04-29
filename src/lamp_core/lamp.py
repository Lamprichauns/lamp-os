import uasyncio as asyncio
import re

# We love lamp.
#
# For some great examples of lamps, see the src/lamps folder
# Lamp.py manages two objects: components and behaviours
# Components are the initial hardware and software dependency setup for your lamp
# Behaviours are the functionality and personality that comprises your lamp
class Lamp():
    def __init__(self, name):
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

    def wake(self):
        asyncio.run(self.start())

    # Start a behaviour
    async def start(self):
        for behaviour in self.behaviours:
            print("Enabling Behaviour: %s" % (behaviour))
            asyncio.create_task(behaviour.run())

        print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep_ms(1)
