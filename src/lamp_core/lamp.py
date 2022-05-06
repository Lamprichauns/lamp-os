import re
import uasyncio as asyncio
from lamp_core.behaviour import StartupBehaviour

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
        self.lock = asyncio.Lock()
        self.name = name
        self.behaviours = []

    # Return a behaviour instance
    def behaviour(self, behaviour_class):
        return next(x for x in self.behaviours if isinstance(x, behaviour_class))

    # Add a behaviour
    def add_behaviour(self, behaviour_class):
        b = behaviour_class(self)

        if any(isinstance(x, behaviour_class) for x in self.behaviours):
            print("Behaviour already added (%s)" % (b))
        else:
            self.behaviours.append(b)
            print("Behaviour added: %s" % (b))

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
            if not isinstance(behaviour, StartupBehaviour):
                print("Enabling Behaviour: %s" % (behaviour))
                asyncio.create_task(behaviour.run())

        print("%s is awake!" % (self.name))

        while True:
            await asyncio.sleep_ms(1)
