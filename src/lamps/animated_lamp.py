import random
import uasyncio as asyncio
from utils.gradient import create_gradient
from utils.fade import fade, pingpong_fade
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from lamp_core.standard_lamp import StandardLamp
from vendor.easing import LinearInOut
from behaviours.warninglights import WarningLights
from behaviours.warpdrive import WarpDrive

# for ease of use, you can define a config to flow into all the components
config = {
    "base":  { "pin": 12, "pixels": 40, "bpp": 3},
    "shade": { "pin": 13, "pixels": 60, "bpp": 3},
    "touch": { "pin": 32 }
}

animated_lamp = StandardLamp("crazybeans", "#5a1700", "#5a1700", config)

animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=30))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=40))
animated_lamp.wake()
