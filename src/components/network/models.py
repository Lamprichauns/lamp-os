import time
import math
from components.network.coding import *

class LampModel:
    def __init__(self, id, name, rssi, attributes={}):
        self.id = id
        self.name = name
        self.rssi = rssi
        self.first_seen = time.time()
        self.last_seen = time.time()

        # Attributes are things about the specific light
        self.attributes = attributes

    def __str__(self) -> str:
        return f"{self.name} rssi: {self.rssi}"

    def __repr__(self) -> str:
        return f'{{id: {id}, name: "{self.name}", rssi: {self.rssi}, first_seen: {self.first_seen}, last_seen: {self.last_seen}}}'

    def update(self, name, rssi, attributes={}) -> bool:
        '''
        Returns `None` if no changes are made, otherwise returns the
        list of changed keys
        '''
        self.last_seen = time.time()

        if self.name == name and self.rssi == rssi and self.attributes == attributes:
            return None

        self.name = name
        self.rssi = rssi

        result = []
        for key, attribute in attributes.items():
            if key in self.attributes and attribute == self.attributes[key]:
                continue

            self.attributes[key] = attribute
            result.append(key)

        return result

class LampNetworkModel:
    ARRIVAL_TIMEOUT = 5
    VISIBLE_TIMEOUT = 15
    STALE_LAMP_TIMEOUT = 30

    def __init__(self):
        self.lamps = {}

    def observed_lamp(self, id, name, rssi, attributes={}) -> bool:
        '''
        Returns `None` if no changes are make, otherwise returns the
        list of changed keys
        '''
        if id in self.lamps:
            return self.lamps[id].update(name, rssi, attributes)
        else:
            self.lamps[id] = LampModel(id, name, rssi, attributes)
            return list(attributes.keys())

    def prune_stale_lamps(self, age=STALE_LAMP_TIMEOUT):
        '''
        age is how many seconds ago they were last seen
        '''
        cutoff = time.time() - age
        ret = filter(lambda elem: elem[1].last_seen > cutoff, self.lamps.items())
        self.lamps = dict(ret)

    @property
    def visible_lamps(self):
        '''
        All lamps first seen within the last `VISIBLE_TIMEOUT` seconds
        '''
        cutoff = time.time() - self.VISIBLE_TIMEOUT
        ret = filter(lambda elem: elem[1].last_seen > cutoff, self.lamps.items())
        return dict(ret)

    @property
    def arrived_lamps(self):
        '''
        All lamps that have shown up since the `VISIBLE_TIMEOUT`
        '''
        cutoff = time.time() - self.ARRIVAL_TIMEOUT
        ret = filter(lambda elem: elem[1].first_seen > cutoff, self.lamps.items())
        return dict(ret)

    @property
    def departed_lamps(self):
        '''
        All lamps that have not shown up since the `STALE_LAMP_TIMEOUT`
        '''
        cutoff = time.time() - self.STALE_LAMP_TIMEOUT
        ret = filter(lambda elem: elem[1].last_seen <= cutoff, self.lamps.items())
        return dict(ret)

class DecayingBroadcastMessage(BroadcastMessage):
    # Number of seconds per TTL tick
    TTL_DURATION = 1

    def __init__(self, code, ttl, payload):
        super().__init__(code, ttl, payload)
        self._created_at = time.time()

    @property
    def ttl(self):
        elapsed = math.floor((time.time() - self._created_at) / self.TTL_DURATION)
        new_ttl = self._ttl - elapsed
        return new_ttl if new_ttl > 0 else 0

class BroadcastMessages(dict):
    def __init__(self, value={}):
        super().__init__(value)

    def set_message(self, code, ttl, payload=None):
        self[code] = DecayingBroadcastMessage(code, ttl, payload)

    # Returns `True` if any messages were deleted otherwise `False`
    def prune_expired_messages(self):
        deleted_messages = []
        for key in list(self.keys()):
            if self[key].ttl == 0:
                del self[key]
                deleted_messages.append(key)

        return deleted_messages

    # Returns `True` if any changes were made
    # A note on Conflict Resolution:
    # If a broadcast message collides with something that that already exist
    # which ever message has a high TTL will take precedence. If they match
    # the existing key will remain.
    #
    # `ttl_adjustment` - The incoming broadcast messages are by default treated with a lower
    # priority than they were sent out with to ensure they decacy over
    # distance to prevent infinite broadcast cycles.
    def update_messages(self, new_messages, ttl_adjustment=1) -> bool:
        did_change = False
        for new_message in new_messages:
            new_message_ttl = new_message.ttl - ttl_adjustment

            if new_message_ttl <= 1:
                continue

            if new_message.code in self and self[new_message.code].ttl >= new_message_ttl:
                continue

            decaying_message = DecayingBroadcastMessage(new_message.code, new_message_ttl, new_message.payload)
            self[new_message.code] = decaying_message
            did_change = True

        return did_change
