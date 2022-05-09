from lamp_core.standard_lamp import StandardLamp
from behaviours.warninglights import WarningLights
from behaviours.warpdrive import WarpDrive
from behaviours.defaults import LampFadeIn

# for ease of use, you can define a config to flow into all the components
config = {
    "base":  { "pin": 12, "pixels": 40, "bpp": 3},
    "shade": { "pin": 13, "pixels": 60, "bpp": 3},
    "touch": { "pin": 32 }
}

animated_lamp = StandardLamp("crazybeans", "#5a1700", "#5a1700", config)

animated_lamp.add_behaviour(LampFadeIn(animated_lamp, frames=6, chained_behaviors=[WarpDrive, WarningLights]))
animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=30))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=60))
animated_lamp.wake()
