import struct
import binascii
import bluetooth
from micropython import const
import ble_advertising as blea


_IRQ_SCAN_RESULT = const(5)
_IRQ_SCAN_DONE = const(6)

_GAP_SCAN_INTERVAL_US = const(128_000)
_GAP_SCAN_WINDOW_US = const(11_250)
_GAP_ADV_INTERVAL_US = const(100_000)

class LampBluetooth:
    MAGIC_NUM = const(42069)
    COLORS_FIELD_FORMAT = "<H3B3B"
    @classmethod
    def _hex_to_rgb(cls, color: str):
        hex_color = color.lstrip('#')
        return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

    @classmethod
    def _rgb_to_hex(cls, rgb_color):
        rgb_bytes = bytes(rgb_color)
        return "#" + binascii.hexlify(rgb_bytes).decode()

    @classmethod
    def _pack_colors_field(cls, base_color, shade_color):
        return struct.pack(cls.COLORS_FIELD_FORMAT,
            cls.MAGIC_NUM,
            *cls._hex_to_rgb(base_color)+cls._hex_to_rgb(shade_color
            )
        )

    @classmethod
    def _is_lamp(cls, field_data):
        # pylint: disable=unsubscriptable-object
        return struct.unpack("<H", field_data)[0] == cls.MAGIC_NUM

    @classmethod
    def _unpack_colors_field(cls, field_data):
        # pylint: disable=unpacking-non-sequence
        (magic, br, bg, bb, sr, sg, sb) = struct.unpack("<H3B3B", field_data)
        if not magic == cls.MAGIC_NUM:
            raise ValueError("Invalid magic number!")
        return ( cls._rgb_to_hex((br, bg, bb)), cls._rgb_to_hex((sr, sg, sb)) )


    # While theoretically, a GAP advertisement can contain any data that will fit within it,
    # after testing, it appears that the it must contain at least the standard header format
    # or the advertisement will be completely ignored by the receiving stack.
    # Therefore, I've settled on an almost-standard payload that contains the name (bonus:
    # the lamp will show up with its designated name on your phone!) and the "vendor-defined"
    # field of "0xFF". This is supposed to begin with an "assigned" vendor number, but instead
    # I'm just using a completely meaningless and random number for the magic number which will
    # allow lamps to recognize each other. It is then followed by the base and shade colors, each
    # packed into 3 bytes each.

    def __init__(self, name, base_color, shade_color) -> None:
        self.ble = bluetooth.BLE()
        self.ble.active(True)
        self.ble.irq(self.bt_irq)
        custom_field = self._pack_colors_field(base_color, shade_color)

        self.adv_payload = blea.advertising_payload(
            name = name,
            custom_fields=( (0xFF, custom_field), )
            )
        print("Payload is ", len(self.adv_payload), " len")
        print("BT Inited")

    def bt_scan(self, on : bool):
        if on:
            self.ble.gap_scan(0, _GAP_SCAN_INTERVAL_US, _GAP_SCAN_WINDOW_US, False)
            print("scanning on")
        else:
            self.ble.gap_scan(None)
            print("scanning off")

    def bt_adv(self, on : bool):
        adv_data = self.adv_payload if on else None
        self.ble.gap_advertise(_GAP_ADV_INTERVAL_US, adv_data, connectable=False)
        print("Adv on: ", on)

    # pylint: disable=too-many-arguments,unused-argument
    def handle_scan_result(self, addr_type, addr, adv_type, rssi, adv_data):
        adv_bytes = bytes(adv_data)
        name = blea.decode_name(adv_bytes)
        if name:
            print("Found named device:", name, "rssi:", str(rssi))
        custom_fields = blea.decode_field(adv_bytes, 0xFF)
        for field in custom_fields:
            if self._is_lamp(field):
                (base_color, shade_color) = self._unpack_colors_field(field)
                print("Found a lamp!", name, base_color, shade_color, "rssi:", rssi)


    def bt_irq(self, event, data):
        if event == _IRQ_SCAN_RESULT:
            # A single scan result.
            addr_type, addr, adv_type, rssi, adv_data = data
            self.handle_scan_result(addr_type, addr, adv_type, rssi, adv_data)
        elif event == _IRQ_SCAN_DONE:
            # Scan duration finished or manually stopped.
            print("_IRQ_SCAN_DONE")
