# Behaviours make up the asynchronous tasks performed by the lamp
class Behaviour:
    def __init__(self, lamp):
        self.lamp = lamp

    def __str__(self):
        return self.__class__.__name__

    async def run(self):
        pass

# A stanadard priority level behavior that will loop indefinitely
class BackgroundBehaviour(Behaviour):
    pass

# A high priority level behavior that will loop indefinitely
class BlockingBehaviour(Behaviour):
    pass

# A behaviour that happens exactly one time at startup
class StartupBehaviour(Behaviour):
    pass
