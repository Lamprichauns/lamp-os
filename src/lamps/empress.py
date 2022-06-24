# Empress is a fine china teapot lamp
from lamp_core.standard_lamp import StandardLamp
from behaviours.social import SocialGreeting

config = {
    "base":  { "pin": 12 },
    "shade": { "pin": 13 }
}

empress = StandardLamp("empress", "#068f13", "#c10ff7", config)
empress.add_behaviour(SocialGreeting(empress))
empress.wake()
