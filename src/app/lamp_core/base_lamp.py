from ..network.coding import Codes
from ..network.network import LampNetworkObserver

class BaseLamp(LampNetworkObserver):
    '''
    Trival Lamp framework that real lamps will be based on.
    Provides a simple interface for the rest of the system to build
    up the lamp and does basic broadcasting about the lamp being created.
    '''
    def __init__(self, network, base_color, shade_color):
        self.network = network

        self.base_color = self._hex_to_rgb(base_color)
        self.shade_color = self._hex_to_rgb(shade_color)

        self.network.announce_attribute(Codes.BASE_COLOR, self.base_color)
        self.network.announce_attribute(Codes.SHADE_COLOR, self.shade_color)

    async def update(self):
        '''
        Every 'Frame' this method is called to update our lights
        and lamp experience. This should be called at a high
        30+ (ideally 60+) framerate.
        '''
        pass

    def _hex_to_rgb(self, value):
        value = value.lstrip('#')
        rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
        return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)
