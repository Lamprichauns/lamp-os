# Adapted from: https://github.com/micropython/micropython/blob/master/examples/bluetooth/ble_advertising.py

import struct
import sys
if sys.implementation.name == "micropython":
    from micropython import const
else:
    def const(val): return val

_ADV_TYPE_FLAGS = const(0x01)
_ADV_TYPE_NAME = const(0x09)
_ADV_TYPE_MANUFACTURER_SPECIFIC_DATA = const(0xff)
_NAME_MAX_LEN = const(6)
_ADV_DATA_MAX_LEN = const(31)

class PayloadPacker:
    def __init__(self, name, magic_number, max_message_length):
        self.name = name
        self.magic_number = magic_number
        self.max_message_length = max_message_length

    def _pack_messages_with_size(self, messages):
        payload = bytearray()

        messages_packed = 0
        for message in messages:
            size_with_next_message = len(payload) + len(message) + 1
            if size_with_next_message >= self.max_message_length:
                break

            payload += struct.pack("B", len(message) + 1) + message
            messages_packed += 1

        return payload, messages_packed

    def build_payload(self, messages):
        payload = bytearray()

        def _append(adv_type, value):
            nonlocal payload
            payload += struct.pack("BB", len(value) + 1, adv_type) + value

        def _append_with_magic(magic, value):
            nonlocal payload
            adv_type = _ADV_TYPE_MANUFACTURER_SPECIFIC_DATA
            payload += struct.pack("<BBH", len(value) + 3, adv_type, magic) + value

        _append(
            _ADV_TYPE_FLAGS,
            # Adapted from: (0x01 if limited_disc else 0x02) + (0x18 if br_edr else 0x04)
            struct.pack("B", 0x06),
        )

        _append(_ADV_TYPE_NAME, self.name.encode('ascii'))

        messages_packed = 0
        if len(messages) > 0:
            packed_messages, messages_packed = self._pack_messages_with_size(messages)
            _append_with_magic(self.magic_number, packed_messages)

        return payload, messages_packed

class DecodedPayload:
    def __init__(self, addr, adv_data, magic_number):
        adv_bytes = bytes(adv_data)

        self.name = self._decode_name(adv_bytes)
        properties = self._decode_custom_properties(magic_number, adv_bytes)
        self.has_messages = False

        if len(properties) > 0:
            self.address = self._addr_as_int(addr)
            self.messages = self._messages_from_payload(properties[0])
            self.has_messages = len(self.messages) > 0

    def _addr_as_int(self, addr):
        vals = struct.unpack("<HL", addr)
        return vals[0] << 32 | vals[1]

    def _messages_from_payload(self, payload):
        i = 0
        result = []
        while i < len(payload):
            size = payload[i]
            result.append(payload[i + 1 : i + size])
            i += size
        return result

    def _decode_field(self, payload, adv_type):
        i = 0
        result = []
        while i + 1 < len(payload):
            if payload[i + 1] == adv_type:
                result.append(payload[i + 2 : i + payload[i] + 1])
            i += 1 + payload[i]
        return result

    def _decode_name(self, payload):
        n = self._decode_field(payload, _ADV_TYPE_NAME)
        return str(n[0], "utf-8") if n else ""

    def _decode_custom_properties(self, magic, payload):
        i = 0
        result = []
        adv_type = _ADV_TYPE_MANUFACTURER_SPECIFIC_DATA
        while i + 1 < len(payload):
            if payload[i + 1] != adv_type:
                i += 1 + payload[i]
                continue
            header = struct.unpack("<H", payload[i + 2 : i + 4])[0]
            if header == magic:
                result.append(payload[i + 4 : i + payload[i] + 1])
            i += 1 + payload[i]
        return result

class PayloadCycler:
    def __init__(self, name, magic_number):
        if len(name) > _NAME_MAX_LEN:
            name = name[0:_NAME_MAX_LEN]
            print(f"WARNING: Name was truncated to '{name}'")

        # Size of message less the data that goes in the advertising header
        max_message_length = _ADV_DATA_MAX_LEN - (len(name) + 9)
        self.packer = PayloadPacker(name, magic_number, max_message_length)

        self.messages = []
        self._message_index = 0

    def next_payload(self):
        self._message_index %= len(self.messages)
        messages = self.messages[self._message_index:] + self.messages[0:self._message_index]
        payload, messages_packed = self.packer.build_payload(messages)

        self._message_index += messages_packed

        return payload
