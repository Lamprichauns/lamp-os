from lamp import Lamp

lamp = Lamp("gramp", "#00ff00", "#ffffff")

def draw_shade():
    print("drawing shade color")


#lamp.register_callback(lambda: draw_shade())

lamp.wake()
