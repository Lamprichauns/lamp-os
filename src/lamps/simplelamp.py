# An example of a simple lamp on standard housing hardware with all basic behaviors enabled
from lamp_core.standard_lamp import StandardLamp

simple = StandardLamp(name="simple", base_color="#40b000", shade_color="#222222")
simple.wake()