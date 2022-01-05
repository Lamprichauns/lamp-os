from behaviour import Behaviour
from lamp import Lamp
import uasyncio as asyncio

andy = Lamp("andy", "#0000ff", "#ff0000")
andy.wake()