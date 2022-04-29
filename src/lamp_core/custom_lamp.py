from lamp_core.lamp import Lamp

# Use custom lamp to run lampos on any hardware configuration you have
class CustomLamp(Lamp):
    def __init__(self, name):
        super().__init__(name)
