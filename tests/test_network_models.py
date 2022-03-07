import time
from app.network.coding import *
from app.network.models import *

# ------------- LampNetworkModel --------------

def test_observing_lamps():
    network = LampNetworkModel()
    network.observed_lamp(1, "lamp1", 10)
    network.observed_lamp(2, "lamp2", 10)

    assert len(network.lamps) == 2

    network.observed_lamp(2, "lamp2", 10)
    assert len(network.lamps) == 2

def test_visible_lamps():
    network = LampNetworkModel()
    network.observed_lamp(1, "lamp1", 10)
    network.observed_lamp(2, "lamp2", 10)
    network.observed_lamp(3, "lamp3", 10)

    assert len(network.lamps) == 3
    assert len(network.visible_lamps) == 3

    network.lamps[3].last_seen = time.time() - 200
    assert len(network.visible_lamps) == 2

def test_arrived_lamps():
    network = LampNetworkModel()
    network.observed_lamp(1, "lamp1", 10)
    network.observed_lamp(2, "lamp2", 10)
    network.observed_lamp(3, "lamp3", 10)

    assert len(network.lamps) == 3
    assert len(network.arrived_lamps) == 3

    network.lamps[3].first_seen = time.time() - 200
    assert len(network.arrived_lamps) == 2
    assert len(network.visible_lamps) == 3

def test_departed_lamps():
    network = LampNetworkModel()
    network.observed_lamp(1, "lamp1", 10)
    network.observed_lamp(2, "lamp2", 10)
    network.observed_lamp(3, "lamp3", 10)

    assert len(network.lamps) == 3
    assert len(network.departed_lamps) == 0

    network.lamps[3].last_seen = time.time() - 200
    assert len(network.departed_lamps) == 1

def test_prune_lamps():
    network = LampNetworkModel()
    network.observed_lamp(1, "lamp1", 10)
    network.observed_lamp(2, "lamp2", 10)
    network.observed_lamp(3, "lamp3", 10)

    assert len(network.lamps) == 3

    network.lamps[3].last_seen = time.time() - 200
    network.prune_stale_lamps()

    assert len(network.lamps) == 2

def test_observed_lamp_reports_changed_attributes():
    network = LampNetworkModel()

    attrs = {
        Codes.BASE_COLOR: LampAttribute(Codes.BASE_COLOR, (5, 6, 7, 8)).encode()}

    keys = network.observed_lamp(1, "lamp1", 10, attrs)

    assert len(keys) == 1
    assert keys[0] == Codes.BASE_COLOR

    keys = network.observed_lamp(1, "lamp1", 10, attrs)

    assert keys == None

    attrs = {
        Codes.SHADE_COLOR: LampAttribute(Codes.SHADE_COLOR, (5, 6, 7, 8)).encode()}

    keys = network.observed_lamp(1, "lamp1", 10, attrs)

    assert len(keys) == 1
    assert keys[0] == Codes.SHADE_COLOR

    assert len(network.lamps[1].attributes) == 2

# ------------ BroadcastMessages -------------

def test_setting_broadcast_messages():
    messages = BroadcastMessages()
    
    messages.set_message(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66))
    messages.set_message(Codes.SHADE_OVERRIDE, 5)

    assert len(messages) == 2
    assert messages[Codes.BASE_OVERRIDE].payload == (99, 88, 77, 66)

def test_update_message_decreases_incoming_ttl():
    messages = BroadcastMessages()
    
    messages.set_message(Codes.BASE_OVERRIDE, 11, (99, 88, 77, 66))
    message2 = BroadcastMessage(Codes.SHADE_OVERRIDE, 5)

    assert messages.update_messages([message2]) == True
    assert messages[message2.code].ttl == 4

    message2 = BroadcastMessage(Codes.SHADE_OVERRIDE, 7)

    assert messages.update_messages([message2], 2) == True
    assert messages[message2.code].ttl == 5

def test_decreasing_broadcast_ttl():
    messages = BroadcastMessages()
    
    messages.set_message(Codes.BASE_OVERRIDE, 4, (99, 88, 77, 66))
    messages.set_message(Codes.SHADE_OVERRIDE, 3)

    messages[Codes.BASE_OVERRIDE]._created_at -= 1
    messages[Codes.SHADE_OVERRIDE]._created_at -= 1

    assert messages[Codes.BASE_OVERRIDE].ttl == 3
    assert messages[Codes.SHADE_OVERRIDE].ttl == 2

    messages[Codes.BASE_OVERRIDE]._created_at -= 2
    messages[Codes.SHADE_OVERRIDE]._created_at -= 2

    messages.prune_expired_messages()

    assert len(messages) == 1
