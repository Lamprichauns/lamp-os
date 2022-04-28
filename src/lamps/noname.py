from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

config = {
    "base": { "pin": 13, "pixels": 40},
    "shade": { "pin": 12, "pixels": 40},
    "touch": { "pin": 32 }
}
thislamp = Lamp("nonameb", "#8314B4", "#ffffff", config)
thislamp.wake()
