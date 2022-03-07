import struct
from app.network.coding import *
from app.network.ble_advertising import *

def test_packing_encoded_messages_adds_sizes():
    packer = PayloadPacker("test", 0x1234, 20)
    messages = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode()]

    bytes, _ = packer._pack_messages_with_size(messages)

    assert len(bytes) == 19
    assert bytes[0] == 6
    assert bytes[12] == 7

def test_packing_encoded_messages_returns_number_of_packed_messages():
    packer = PayloadPacker("test", 0x1234, 16)
    messages = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode(),
        BroadcastMessage(Codes.SHADE_OVERRIDE, 11, (1, 2, 3, 4)).encode()]

    bytes, messages_packed = packer._pack_messages_with_size(messages)

    assert len(bytes) == 12
    assert messages_packed == 2

def test_packing_encodes_name_and_magic_number():
    packer = PayloadPacker("test", 0x1234, 20)
    messages = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode()]

    bytes, _ = packer.build_payload(messages)

    assert bytes[5 : 9] == "test".encode('ascii')
    assert bytes[11] == 0x34
    assert bytes[12] == 0x12

def test_decoder_parsing_data():
    packer = PayloadPacker("test", 0x1234, 20)
    messages = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode()]

    bytes, _ = packer.build_payload(messages)
    addr = struct.pack("Q", 0xffeeddccbbaa)[0:6]
    decoder = DecodedPayload(addr, bytes, 0x1234)

    assert decoder.name == "test"
    assert len(decoder.messages) == 3
    assert decoder.messages[0][0] == Codes.BASE_COLOR
    assert decoder.messages[2][0] == Codes.BASE_OVERRIDE

def test_decoder_parsing_bad_data():
    packer = PayloadPacker("test", 0x1234, 20)
    messages = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode()]

    bytes, _ = packer.build_payload(messages)
    addr = struct.pack("Q", 0xffeeddccbbaa)[0:6]

    for i in range(0, 10):
        bytes[16 + i] = 0xfe

    decoder = DecodedPayload(addr, bytes, 0x1234)

    assert decoder.name == "test"
    assert len(decoder.messages) == 2
    assert decoder.messages[0][0] == Codes.BASE_COLOR

def test_cycling_moves_through_messages():
    cycler = PayloadCycler("Tester", 0x1234)
    messages = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode(),
        BroadcastMessage(Codes.SHADE_OVERRIDE, 11, (1, 2, 3, 4)).encode(),
        BroadcastMessage(0x99, 11, (6, 7, 8, 9)).encode()]

    cycler.messages = messages

    payload = cycler.next_payload()

    payload_start = 15

    assert len(payload) == 27
    assert payload[payload_start + 1] == Codes.BASE_COLOR
    assert payload[payload_start + 7] == Codes.SHADE_COLOR

    payload = cycler.next_payload()

    assert len(payload) == 29
    assert payload[payload_start + 1] == Codes.BASE_OVERRIDE
    assert payload[payload_start + 8] == Codes.SHADE_OVERRIDE
    
    payload = cycler.next_payload()

    assert len(payload) == 28
    assert payload[payload_start + 1] == 0x99
    assert payload[payload_start + 8] == Codes.BASE_COLOR
