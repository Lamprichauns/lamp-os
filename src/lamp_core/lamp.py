import re
import uasyncio as asyncio
from lamp_core.behaviour import StartupBehaviour, AnimatedBehaviour, ControllerBehaviour

# We love lamp.
#
# Please use SimpleLamp or CustomLamp configurations from lamp_core instead of using Lamp directly
# Check the following examples in lamps for usage
# simplelamp:   A standard lamp example with a base, shade and networking
# customlamp:  A fully custom lamp example with different components, custom behaviors, etc.
class Lamp():
    def __init__(self, name):
        if not re.match('^[a-z]+$', name):
            raise NameError('Name must be lowercase alpha')
        self.name = name
        self.behaviours = []

    # Return a behaviour instance
    def behaviour(self, behaviour_class):
        return next(x for x in self.behaviours if isinstance(x, behaviour_class))

    # Add a behaviour
    def add_behaviour(self, behaviour_class):
        self.behaviours.append(behaviour_class)
        print("Behaviour added: %s" % (behaviour_class))

    # Called once all components and behaviours added to begin all async tasks
    def wake(self):
        asyncio.run(self.start())

    # Start all behaviours
    async def start(self):
        for behaviour in self.behaviours:
            if isinstance(behaviour, StartupBehaviour):
                print("Running Startup Behaviour: %s" % (behaviour))
                await behaviour.run()

        for behaviour in self.behaviours:
            if isinstance(behaviour, AnimatedBehaviour):
                print("Enabling Animation: %s" % (behaviour))
                asyncio.create_task(behaviour.animate())

            if isinstance(behaviour, ControllerBehaviour):
                print("Enabling Behavior: %s" % (behaviour))
                asyncio.create_task(behaviour.run())

        print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep(0)
