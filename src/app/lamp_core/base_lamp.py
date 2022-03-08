from ..network.coding import Codes
from ..network.network import LampNetworkObserver
from ..colors import *

class BaseLamp(LampNetworkObserver):
    '''
    Trival Lamp framework that real lamps will be based on.
    Provides a simple interface for the rest of the system to build
    up the lamp and does basic broadcasting about the lamp being created.
    '''
    def __init__(self, network, base_color, shade_color):
        self.network = network

        self.base_color = RGBW.from_(base_color)
        self.shade_color = RGBW.from_(shade_color)

        self.network.announce_attribute(Codes.BASE_COLOR, self.base_color.raw)
        self.network.announce_attribute(Codes.SHADE_COLOR, self.shade_color.raw)

    async def update(self):
        '''
        Every 'Frame' this method is called to update our lights
        and lamp experience. This should be called at a high
        30+ (ideally 60+) framerate.
        '''
        pass
    