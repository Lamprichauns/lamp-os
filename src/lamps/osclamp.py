import uasyncio as asyncio
from lamp_core.standard_lamp import StandardLamp
from lamp_core.behaviour import BackgroundBehavior, AnimatedBehaviour
from utils.gradient import create_gradient
from vendor.uosc.async_server import async_osc, OSCMessage
from components.network.access_point import AccessPoint
from behaviours.lamp_fade_in import LampFadeIn
from behaviours.lamp_idle import LampIdle
from components.network.bluetooth import Bluetooth

config = {
    "shade": { "pin": 13, "pixels": 12 },
    "lamp": { "name": "osc", "default_behaviours": "false" },
    "wifi": { "ssid": "lamp-290712" }
}

osc_state = OSCMessage()

class OSCControl(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.previous = ""

    async def draw(self):
        await self.next_frame()

    async def control(self):
        while True:
            await asyncio.sleep(1)
            if osc_state.arguments != self.previous:
                print(osc_state.arguments)
                self.lamp.bluetooth.ble.active(False)
                await asyncio.sleep(4)
                self.lamp.bluetooth = Bluetooth("osc", "#FF0000", "#00FF00")
                self.lamp.network = self.lamp.bluetooth.network
                self.lamp.bluetooth.enable()

            self.previous = osc_state.arguments

# Start listening for connections on port 57121
class OscListener(BackgroundBehavior):
    async def run(self):
        await asyncio.sleep(0)
        asyncio.run(async_osc('0.0.0.0', 57121, dispatch=osc_state))

# high speed animation to see the effects of latency caused by rapid socket access
class WarpDrive(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.warp_drive_pattern = create_gradient((35, 7, 0, 0), (90, 23, 0, 0), self.lamp.shade.num_pixels)

    async def draw(self):
        self.warp_drive_pattern = self.warp_drive_pattern[3:] + self.warp_drive_pattern[:3]

        for i in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[i] = self.warp_drive_pattern[i]
        await self.next_frame()

osc = StandardLamp(name="osc", base_color="#126674", shade_color="#111111", config_opts=config)
osc.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")
osc.add_behaviour(LampFadeIn(osc, frames=30, chained_behaviors = [LampIdle, WarpDrive, OSCControl]))
osc.add_behaviour(LampIdle(osc, frames=1))
osc.add_behaviour(WarpDrive(osc, frames=30))
osc.add_behaviour(OSCControl(osc, frames=30))
osc.add_behaviour(OscListener(osc))
osc.wake()
