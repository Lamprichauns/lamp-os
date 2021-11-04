import binascii, hashlib

# Simple Lamp parent class. This is extended by Lamp, as well as used to track other Lamps that are nearby.
class SimpleLamp:
    def __init__(self, name, base_color, shade_color):
        self.name = name
        self.base_color = base_color
        self.shade_color = shade_color

        self.callback = None

        attrs = "{name}-{base_color}-{shade_color}".format(name = self.name, base_color = self.base_color, shade_color = self.shade_color)
        sha = hashlib.sha1(attrs)
        self.lamp_id = binascii.hexlify(sha.digest()).decode()

    def __eq__(self,other):
        return isinstance(other, SimpleLamp) and self.lamp_id == other.lamp_id