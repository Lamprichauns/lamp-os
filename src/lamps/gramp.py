from lamp import Lamp

lamp = Lamp("gramp", "#884400", "#00ffff")

def draw_shade(lamp):
    # This is just for testing, but makes gramp steal and invert the most recently arrived other lamps colors
    if lamp.lamp_network["current"]:
        lamp.adjust_shade(lamp.lamp_network["current"][-1].base_color)
        lamp.adjust_base(lamp.lamp_network["current"][-1].shade_color)
    else:
        lamp.reset_lights()

lamp.register_callback(lambda: draw_shade(lamp))

lamp.wake()
