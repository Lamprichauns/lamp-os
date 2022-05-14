# A barebones lamp with configuration and social features
from behaviours.social import SocialGreeting
from lamp_core.standard_lamp import StandardLamp

# Override the standard lamp configs if necessary
config = {
    "base":  { "pin": 12 },
    "shade": { "pin": 13 }
}

mclamp = StandardLamp("mclamp", "70964b", "#ffffff", config)

# Include more behaviours here and leave social components in the last position
mclamp.add_behaviour(SocialGreeting(mclamp, frames=3000))

mclamp.wake()
