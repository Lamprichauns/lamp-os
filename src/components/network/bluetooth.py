import struct
import binascii
import time
import bluetooth
from micropython import const
import uasyncio as asyncio
import components.network.ble_advertising as blea

_IRQ_SCAN_RESULT = const(5)
_IRQ_SCAN_DONE = const(6)

# Scan every INTERVAL for WINDOW
_GAP_SCAN_INTERVAL_US = const(20_000)
_GAP_SCAN_WINDOW_US = const(10_000)

# Advertise every INTERVAL
_GAP_ADV_INTERVAL_US = const(100_000)

# :TODO: Update the internal rgb/hex conversion to work with the W pixel and remove this
def hex_to_rgbw(value):
    value = value.lstrip('#')
    rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
    return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)

class LampNetwork:
    def __init__(self):
        self.lamps = {}
        self.departed_lamps = {}
        self.arrived_lamps = {}

    def found(self, name, base_color, shade_color, rssi):
        if name in self.lamps:
            self.lamps[name]["rssi"] = rssi
            self.lamps[name]["last_seen"] = time.time()
        else:
            print("RRSI of lamp %s" % (rssi))
            if rssi > -94: # Don't add it unless it's strong enough of a signal.
                print("BT: New lamp found %s (%s, %s @%s)" % (name, base_color, shade_color,rssi))
                self.arrived_lamps[name] = self.lamps[name] = { "base_color": hex_to_rgbw(base_color), "shade_color": hex_to_rgbw(shade_color), "rssi": rssi, "first_seen": time.time(), "last_seen": time.time() }

    # returns departed lamps. Optionally pass the name of a lamp to only look for that lamp
    @classmethod
    async def _await_lamps(cls, name, lst):
        if name is None:
            while not any(lst):
                await asyncio.sleep(0)

            lamp = lst.popitem()
        else:
            while name not in lst:
                await asyncio.sleep(0)

            lamp = (name, lst.pop(name))

        return {"name": lamp[0], "base_color": lamp[1]["base_color"], "shade_color": lamp[1]["shade_color"]}

    @classmethod
    def _prune_lamp_list(cls, lst, field, timeout):
        for name, lamp in lst.items():
            if time.time() - lamp[field] >= timeout:
                lst.pop(name)

    # await for and return departed lamps. Optionally specify the name to look for a specific lamp
    async def departed(self, name = None):
        return await self._await_lamps(name, self.departed_lamps)

    # await for and return arrived lamps. Optionally specifiy the name to look for a specific lamp
    async def arrived(self, name = None):
        return await self._await_lamps(name, self.arrived_lamps)

    async def monitor(self):
        while True:
            await asyncio.sleep(1)

            # Add timed out lamps to departed list
            for name, lamp in self.lamps.items():
                if time.time() - lamp["last_seen"] >= 5: # Currently with less timeout than this it will sometimes get un-seen/re-seen
                    self.departed_lamps[name] = self.lamps.pop(name)
                    print("BT: %s has left, removing" % (name))

            self._prune_lamp_list(self.departed_lamps, "last_seen", 30)
            self._prune_lamp_list(self.arrived_lamps, "first_seen", 15)


class Bluetooth:
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
        # pylint: disable=unpacking-non-sequence,invalid-name
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
        self.network = LampNetwork()

        self.set_payload(name, base_color, shade_color)

        print("BT Initialized")

    def set_payload(self, name, base_color, shade_color):
        custom_field = self._pack_colors_field(base_color, shade_color)

        self.adv_payload = blea.advertising_payload(
            name = name,
            custom_fields=( (0xFF, custom_field), )
            )

    def enable(self):
        self.ble.gap_scan(0, _GAP_SCAN_INTERVAL_US, _GAP_SCAN_WINDOW_US, False)
        self.ble.gap_advertise(_GAP_ADV_INTERVAL_US, self.adv_payload, connectable=False)
        asyncio.create_task(self.network.monitor())

        print("BT enabled (scanning & advertising)")


    # pylint: disable=too-many-arguments,unused-argument
    def handle_scan_result(self, addr_type, addr, adv_type, rssi, adv_data):
        try:
            adv_bytes = bytes(adv_data)
            name = blea.decode_name(adv_bytes)

            custom_fields = blea.decode_field(adv_bytes, 0xFF)
            for field in custom_fields:
                if self._is_lamp(field):
                    (base_color, shade_color) = self._unpack_colors_field(field)
                    self.network.found(name, base_color, shade_color, rssi)

        except Exception as err:
            print("IRQ handler failed to decode scan result: ", err)

    def bt_irq(self, event, data):
        if event == _IRQ_SCAN_RESULT:
            # A single scan result.
            addr_type, addr, adv_type, rssi, adv_data = data
            self.handle_scan_result(addr_type, addr, adv_type, rssi, adv_data)
        elif event == _IRQ_SCAN_DONE:
            # Scan duration finished or manually stopped.
            print("_IRQ_SCAN_DONE")
