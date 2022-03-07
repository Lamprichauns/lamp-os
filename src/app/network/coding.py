import struct

class Codes:
    # Attribute Codes
    VERSION     = 0x00 # 16-bit version
    BASE_COLOR  = 0x01 # r, g, b, w
    SHADE_COLOR = 0x02 # r, g, b, w

    # Broadcast Codes
    BASE_OVERRIDE   = 0x91
    SHADE_OVERRIDE  = 0x92

class LampAttribute:
    def __init__(self, code, value):
        self.code = code
        self.value = value

    def __repr__(self) -> str:
        return f'{{code: {self.code}, value: {self.value}}}'

    def __eq__(self, rhs):
        return self.code == rhs.code and self.value == rhs.value

    def __ne__(self, rhs):
        return self.code != rhs.code or self.value != rhs.value

    def encode(self):
        if self.code == Codes.VERSION:
            return struct.pack(f'>BH', self.code, self.value)
        return struct.pack(f'B{len(self.value)}B', self.code, *self.value)

    @classmethod
    def decode(cls, data):
        if data[0] == Codes.VERSION:
            return LampAttribute(data[0], (data[1] << 8 | data[2]))
        return LampAttribute(data[0], tuple(data[1:]))

class BroadcastMessage:
    def __init__(self, code, ttl, payload=None):
        self.code = code
        self._ttl = ttl
        self.payload = payload

    def __repr__(self) -> str:
        return f'{{code: {self.code}, ttl: {self.ttl}, payload: {self.payload}}}'

    def __eq__(self, rhs):
        return self.code == rhs.code and self.ttl == rhs.ttl and self.payload == rhs.payload

    def __ne__(self, rhs):
        return self.code != rhs.code or self.value != rhs.value or self.payload != rhs.payload

    @property
    def ttl(self):
        return self._ttl
    
    @ttl.setter
    def ttl(self, val):
        self._ttl = val

    def encode(self):
        if self.payload == None:
            return struct.pack(f'BB', self.code, self.ttl)

        # Format: Code, TTL, Raw Payload
        return struct.pack(f'BB{len(self.payload)}B', self.code, self.ttl, *self.payload)

    @classmethod
    def decode(cls, data):
        if len(data) == 2:
            return BroadcastMessage(data[0], data[1])

        return BroadcastMessage(data[0], data[1], tuple(data[2:]))
