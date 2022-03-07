import struct
from app.network.coding import *

def test_encode_VERSION():
    val = LampAttribute(Codes.VERSION, 415).encode()
    
    assert len(val) == 3
    assert val[0] == Codes.VERSION
    assert val[1] == 1
    assert val[2] == 159

def test_encode_BASE_COLOR():
    val = LampAttribute(Codes.BASE_COLOR, (200, 100, 50, 25)).encode()
    
    assert len(val) == 5
    assert val[0] == Codes.BASE_COLOR
    assert val[1] == 200
    assert val[2] == 100
    assert val[3] == 50
    assert val[4] == 25

def test_encode_SHADE_COLOR():
    val = LampAttribute(Codes.SHADE_COLOR, (88, 99, 111, 222)).encode()
    
    assert len(val) == 5
    assert val[0] == Codes.SHADE_COLOR
    assert val[1] == 88
    assert val[2] == 99
    assert val[3] == 111
    assert val[4] == 222

def test_encode_BASE_OVERRIDE():
    val = BroadcastMessage(Codes.BASE_OVERRIDE, 7, (200, 100, 50, 25)).encode()
    
    assert len(val) == 6
    assert val[0] == Codes.BASE_OVERRIDE
    assert val[1] == 7
    assert val[2] == 200
    assert val[3] == 100
    assert val[4] == 50
    assert val[5] == 25

def test_encode_SHADE_OVERRIDE():
    val = BroadcastMessage(Codes.SHADE_OVERRIDE, 8).encode()

    assert len(val) == 2
    assert val[0] == Codes.SHADE_OVERRIDE
    assert val[1] == 8

def test_decode_VERSION():
    data = struct.pack("BBB", Codes.VERSION, 1, 159)
    attribute = LampAttribute.decode(data)

    assert attribute.code == Codes.VERSION
    assert attribute.value == 415

def test_decode_BASE_COLOR():
    data = struct.pack("BBBBB", Codes.BASE_COLOR, 4, 8, 12, 16)
    attribute = LampAttribute.decode(data)

    assert attribute.code == Codes.BASE_COLOR
    assert attribute.value == (4, 8, 12, 16)

def test_decode_SHADE_COLOR():
    data = struct.pack("BBBBB", Codes.SHADE_COLOR, 5, 6, 7, 8)
    attribute = LampAttribute.decode(data)

    assert attribute.code == Codes.SHADE_COLOR
    assert attribute.value == (5, 6, 7, 8)

def test_decode_BASE_OVERRIDE():
    data = struct.pack("BBBBBB", Codes.BASE_OVERRIDE, 11, 1, 2, 3, 4)
    message = BroadcastMessage.decode(data)

    assert message.code == Codes.BASE_OVERRIDE
    assert message.ttl == 11
    assert message.payload == (1, 2, 3, 4)

def test_decode_SHADE_OVERRIDE():
    data = struct.pack("BB", Codes.SHADE_OVERRIDE, 9)
    message = BroadcastMessage.decode(data)

    assert message.code == Codes.SHADE_OVERRIDE
    assert message.ttl == 9
