from lamp_core.standard_lamp import StandardLamp

config = {
    "base":  { "pin": 12 },
    "shade": { "pin": 13 }
}

mclamp = StandardLamp("mclamp", "70964b", "#ffffff", config)
mclamp.wake()
