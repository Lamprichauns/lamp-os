import asyncio
from app.network.coding import *
from app.network.models import *
from app.network.network import *

# Patch sleep_ms into asyncio for testing as it's a Micropython API variation
async def _sleep_ms(dur):
    await asyncio.sleep(dur / 1000)
asyncio.sleep_ms = _sleep_ms

# ------------- LampNetwork --------------

async def test_observing_lamp():
    network = LampNetwork("Test Lamp")

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        LampAttribute(0x40, (10, 20, 30, 40, 50)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode()]

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert len(network.model.visible_lamps) == 1

    observed_lamp = network.model.visible_lamps[3]
    assert len(observed_lamp.attributes) == 3

async def test_repeating_unknown_broadcast_message():
    network = LampNetwork("Test Lamp")

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode(),
        BroadcastMessage(0xC1, 12, (5, 6, 7, 8, 9, 10)).encode()]

    await network.observed_lamp(3, "New Lamp", 9, properties)

    assert len(network._broadcast_messages) == 2
    assert network._broadcast_messages[Codes.BASE_OVERRIDE].ttl == 10
    assert network._broadcast_messages[0xC1].ttl == 11

class MockLampNetworDelegate(LampNetworkDelegate):
    def __init__(self) -> None:
        self.announce_attributes_data = None
        self.broadcast_messages_data = None

    def announce_attributes(self, attributes):
        self.announce_attributes_data = attributes

    def broadcast_messages(self, messages):
        self.broadcast_messages_data = messages

async def test_update_attributes_fires_on_attribute_change():
    network_tester = MockLampNetworDelegate()
    network = LampNetwork("Test Lamp")
    network.network_delegate = network_tester

    network.announce_attribute(Codes.BASE_COLOR, (5, 6, 7, 8))

    assert len(network_tester.announce_attributes_data) == 1
    assert network_tester.announce_attributes_data[Codes.BASE_COLOR].value == (5, 6, 7, 8)

def test_broadcast_messages_fires_on_send_broadcast():
    mock_delegate = MockLampNetworDelegate()
    network = LampNetwork("Test Lamp")
    network.network_delegate = mock_delegate

    network.send_broadcast(Codes.BASE_OVERRIDE, (99, 88, 77, 66))

    assert len(mock_delegate.broadcast_messages_data) == 1
    assert mock_delegate.broadcast_messages_data[Codes.BASE_OVERRIDE].payload == (99, 88, 77, 66)

async def test_broadcast_messages_fires_when_remote_has_message():
    mock_delegate = MockLampNetworDelegate()
    network = LampNetwork("Test Lamp")
    network.network_delegate = mock_delegate

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode(),
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode(),
        BroadcastMessage(0xC1, 12, (5, 6, 7, 8, 9, 10)).encode()]

    await network.observed_lamp(3, "New Lamp", 9, properties)

    assert len(mock_delegate.broadcast_messages_data) == 2
    assert mock_delegate.broadcast_messages_data[Codes.BASE_OVERRIDE].ttl == 10
    assert mock_delegate.broadcast_messages_data[Codes.BASE_OVERRIDE].payload == (99, 88, 77, 66)

# -------------- LampNetworkObserver Test --------------

class MockLampNetworkObserver(LampNetworkObserver):
    def __init__(self) -> None:
        self.new_lamp_appeared_data = None
        self.lamp_changed_data = None
        self.changed_lamp_attributes = {}
        self.lamps_departed_data = None
        self.messages_data = {}
        self.stopped_messages_data = {}

    async def new_lamp_appeared(self, new_lamp):
        self.new_lamp_appeared_data = new_lamp

    async def lamp_changed(self, lamps):
        self.lamp_changed_data = lamps

    async def lamp_attribute_changed(self, lamp, attribute):
        if lamp.id not in self.changed_lamp_attributes:
            self.changed_lamp_attributes[lamp.id] = {}
        self.changed_lamp_attributes[lamp.id][attribute.code] = attribute

    async def lamps_departed(self, lamps):
        self.lamps_departed_data = lamps

    async def message_observed(self, message):
        self.messages_data[message.code] = message

    async def message_stopped(self, code):
        self.stopped_messages_data[code] = True

def test_add_observer_with_duplicate_observers_adds_only_first():
    mock_observer = MockLampNetworkObserver()
    mock_observer2 = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    network.add_observer(mock_observer)
    assert len(network._observers) == 1

    network.add_observer(mock_observer)
    assert len(network._observers) == 1

    network.add_observer(mock_observer2)
    assert len(network._observers) == 2

def test_remove_observer_with_observers():
    mock_observer = MockLampNetworkObserver()
    mock_observer2 = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    network.add_observer(mock_observer)
    network.add_observer(mock_observer2)
    assert len(network._observers) == 2

    network.remove_observer(mock_observer)
    assert len(network._observers) == 1
    network.remove_observer(mock_observer)
    assert len(network._observers) == 1
    network.remove_observer(mock_observer2)
    assert len(network._observers) == 0

async def test_observer_called_when_new_lamp_is_observed():
    mock_observer = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    network.add_observer(mock_observer)

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode()]

    assert mock_observer.new_lamp_appeared_data == None

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert mock_observer.new_lamp_appeared_data != None
    assert mock_observer.new_lamp_appeared_data.id == 3
    assert mock_observer.new_lamp_appeared_data.name == "Observed Lamp"

async def test_observer_called_when_existing_lamp_changes():
    mock_observer = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    network.add_observer(mock_observer)

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode()]

    assert mock_observer.lamp_changed_data == None

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    properties = [
        LampAttribute(Codes.BASE_COLOR, (50, 100, 150, 200)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode()]

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert mock_observer.lamp_changed_data != None
    assert mock_observer.lamp_changed_data.name == "Observed Lamp"

    mock_observer.lamp_changed_data = None

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert mock_observer.lamp_changed_data == None

    await network.observed_lamp(3, "Observed Lamp", 8, properties)

    assert mock_observer.lamp_changed_data != None
    assert mock_observer.lamp_changed_data.name == "Observed Lamp"

async def test_observer_called_when_existing_lamp_attribute_changes():
    mock_observer = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    network.add_observer(mock_observer)

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode()]

    assert mock_observer.changed_lamp_attributes == {}

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert mock_observer.changed_lamp_attributes[3][Codes.BASE_COLOR].value == (5, 6, 7, 8)

    properties = [
        LampAttribute(Codes.BASE_COLOR, (50, 100, 150, 200)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode()]

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert mock_observer.changed_lamp_attributes[3][Codes.BASE_COLOR].value == (50, 100, 150, 200)
    assert mock_observer.changed_lamp_attributes[3][Codes.SHADE_COLOR].value == (1, 2, 3, 4)

async def test_observer_called_when_lamp_disappears():
    mock_observer = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    await network.start_monitoring()

    network.add_observer(mock_observer)

    properties = [
        LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode(),
        LampAttribute(Codes.SHADE_COLOR, (1, 2, 3, 4)).encode()]

    assert mock_observer.lamps_departed_data == None

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    network.model.lamps[3].last_seen = time.time() - LampNetworkModel.STALE_LAMP_TIMEOUT

    await asyncio.sleep_ms(LampNetwork.MONITOR_INTERVAL_MS)
    assert len(mock_observer.lamps_departed_data) == 1
    assert mock_observer.lamps_departed_data[3].name == "Observed Lamp"

    await asyncio.sleep_ms(LampNetwork.MONITOR_INTERVAL_MS)
    assert len(network.model.lamps) == 0
    # Departed lamps should not be called with 0 lamps
    assert len(mock_observer.lamps_departed_data) == 1

    await network.stop_monitoring()

async def test_observer_called_when_messages_received():
    mock_observer = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    network.add_observer(mock_observer)

    properties = [
        BroadcastMessage(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66)).encode()]

    assert mock_observer.messages_data == {}

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    assert len(mock_observer.messages_data) == 1
    assert mock_observer.messages_data[Codes.BASE_OVERRIDE].payload == (99, 88, 77, 66)

    properties = [
        BroadcastMessage(Codes.BASE_OVERRIDE, 12, (1, 2, 3, 4)).encode()]

    await network.observed_lamp(3, "Observed Lamp", 4, properties)
    assert mock_observer.messages_data[Codes.BASE_OVERRIDE].payload == (1, 2, 3, 4)

async def test_monitoring_prunes_expired_messages_and_observer_is_notified():
    mock_observer = MockLampNetworkObserver()
    network = LampNetwork("Test Lamp")

    await network.start_monitoring()

    network.add_observer(mock_observer)

    properties = [
        BroadcastMessage(Codes.BASE_OVERRIDE, 3, (99, 88, 77, 66)).encode()]

    assert mock_observer.stopped_messages_data == {}

    await network.observed_lamp(3, "Observed Lamp", 4, properties)

    network._broadcast_messages[Codes.BASE_OVERRIDE]._created_at -= 3
    await asyncio.sleep_ms(LampNetwork.MONITOR_INTERVAL_MS)

    assert len(mock_observer.stopped_messages_data) == 1
    assert mock_observer.stopped_messages_data[Codes.BASE_OVERRIDE] == True

    await network.stop_monitoring()
