# This file is only here for development.
# Change this to point at whatever lamp you are working on!

import lamps.gramp


# Create lamps in lamps/yourlamp.py
#
# Here's an example for a simple lamp, this is the bare minimum required for things to work:

#
#   from lamp import Lamp
#   lamp = Lamp("gramp", "#00ff00", "#ffffff")
#   lamp.wake()
#
#
# To do more complicated things, register a callback before calling `lamp.wake()`. This will run
# 4 times per second.
#
# Example code:
#
#   from lamp import Lamp
#   lamp = Lamp("gramp", "#00ff00", "#ffffff")
#
#   def update_lamp(lamp):
#       print("drawing shade color")
#
#   lamp.register_callback(lambda: update_lamp(lamp))
#   lamp.wake()