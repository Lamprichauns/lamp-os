from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio
import random

config = { 
    "base": { "pin": 2, "pixels": 40}, 
    "shade": { "pin": 26, "pixels": 40}, 
    "touch": { "pin": 32 }
} 
ylamp = Lamp("andy", "#F23591", "#ffffff", config)

ylamp.wake()