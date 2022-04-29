import bluetooth
from micropython import const
import uasyncio as asyncio
from components.network.ble_advertising import *
from components.network.network import NetworkDelegate

_IRQ_SCAN_RESULT = const(5)
_IRQ_SCAN_DONE = const(6)

# Scan every INTERVAL for WINDOW
_GAP_SCAN_INTERVAL_US = const(20_000)
_GAP_SCAN_WINDOW_US = const(10_000)

# Advertise every INTERVAL
_GAP_ADV_INTERVAL_US = const(100_000)

class Bluetooth(NetworkDelegate):
    MAGIC_NUMBER = const(42069)
    ADVERTISE_UPDATE_INTERVAL_MS = const(1000)

    # While theoretically, a GAP advertisement can contain any data that will fit within it,
    # after testing, it appears that the it must contain at least the standard header format
    # or the advertisement will be completely ignored by the receiving stack.
    # Therefore, I've settled on an almost-standard payload that contains the name (bonus:
    # the lamp will show up with its designated name on your phone!) and the "vendor-defined"
    # field of "0xFF". This is supposed to begin with an "assigned" vendor number, but instead
    # I'm just using a completely meaningless and random number for the magic number which will
    # allow lamps to recognize each other. It is then followed by the base and shade colors, each
    # packed into 3 bytes each.

    def __init__(self, network) -> None:
        self.ble = bluetooth.BLE()
        self.ble.active(True)
        self.ble.irq(self.bt_irq)
        print("BLE Initialized")

        self.network = network
        self.network.network_delegate = self

        self.advertising_cycler = PayloadCycler(network.name, self.MAGIC_NUMBER)

        self._attributes = {}
        self._broadcast_messages = {}
        self._should_update_advertisement = False

        self._advertising = False
        self._advertising_task = None

    def _update_advertisement(self):
        messages = list(map(lambda x: x.encode(), self._attributes.values())) + \
            list(map(lambda x: x.encode(), self._broadcast_messages.values()))
        self.advertising_cycler.messages = messages
        payload = self.advertising_cycler.next_payload()
        self.ble.gap_advertise(_GAP_ADV_INTERVAL_US, payload, connectable=False)

    def announce_attributes(self, attributes):
        self._attributes = attributes
        self._should_update_advertisement = True

    def broadcast_messages(self, messages):
        self._broadcast_messages = messages
        self._should_update_advertisement = True

    # pylint: disable=too-many-arguments,unused-argument
    def handle_scan_result(self, addr_type, addr, adv_type, rssi, adv_data):
        payload = DecodedPayload(addr, adv_data, self.MAGIC_NUMBER)
        if payload.has_messages:
            task = self.network.observed_lamp(payload.address, payload.name, rssi, payload.messages)
            asyncio.create_task(task)

    def bt_irq(self, event, data):
        if event == _IRQ_SCAN_RESULT:
            # A single scan result.
            addr_type, addr, adv_type, rssi, adv_data = data
            self.handle_scan_result(addr_type, addr, adv_type, rssi, adv_data)
        elif event == _IRQ_SCAN_DONE:
            # Scan duration finished or manually stopped.
            print("_IRQ_SCAN_DONE")

    async def _advertise_update_loop(self):
        while self._advertising:
            total_messages = len(self._broadcast_messages) + len (self._attributes)
            if self._should_update_advertisement or total_messages > 1:
                self._update_advertisement()
                self._should_update_advertisement = False

            await asyncio.sleep_ms(self.ADVERTISE_UPDATE_INTERVAL_MS)

    async def enable(self):
        self.ble.gap_scan(0, _GAP_SCAN_INTERVAL_US, _GAP_SCAN_WINDOW_US, False)
        self._advertising = True
        self._advertising_task = asyncio.create_task(self._advertise_update_loop())
        print("BLE Enabled (Scanning & Advertising)")

    async def disable(self):
        if not self._advertising_task:
            return

        self._advertising = False
        await self._advertising_task
        self._advertising_task = None

        self.ble.gap_scan(active=False)
