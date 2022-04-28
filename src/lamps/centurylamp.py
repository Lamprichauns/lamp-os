from lamp import Lamp

config = {
    "base": { "pin": 12, "pixels": 40 },
    "shade": { "pin": 13, "pixels": 40 },
    "touch": { "pin": 32 },
    "motion": { "pin_sda": 21, "pin_scl": 22, "gravity_axis": "x_axis" }
}

century = Lamp("century", "#5b4711", "#5b4711", config)
century.wake();
