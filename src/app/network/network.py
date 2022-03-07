import sys
if sys.implementation.name == "micropython":
    import uasyncio as asyncio
else:
    import asyncio

from .models import *
from .coding import *

class LampNetworkObserver:
    def __init__(self):
        pass

    async def new_lamp_appeared(self, new_lamp):
        pass

    async def lamp_changed(self, lamp):
        pass

    async def lamp_attribute_changed(self, lamp, attribute):
        pass

    async def lamps_departed(self, lamps):
        pass

    async def message_observed(self, message):
        pass

    async def message_stopped(self, code):
        pass

class LampNetworkDelegate:
    # Called with the list of LampAttributes
    def announce_attributes(self, attributes):
        pass

    # Called with the list of BroadcastMessages
    def broadcast_messages(self, messages):
        pass

class LampNetwork:
    # How much delay between monitor cycles
    MONITOR_INTERVAL_MS = 50

    def __init__(self, name):
        self.model = LampNetworkModel()
        self.network_delegate = None

        self._name = name

        # List of attributes to be visible to other lamps
        self._attributes = {}

        # Broadcast messages are the collection of all the messages agrigated
        # from all the lamps that are viewed. This list will be re-broadcast        
        # from this lamp.
        self._broadcast_messages = BroadcastMessages()

        self._observers = []

        self._monitoring = False
        self._monitor_task = None

    def announce_attribute(self, code, val):
        self._attributes[code] = LampAttribute(code, val)
        if self.network_delegate != None:
            self.network_delegate.announce_attributes(self._attributes)

    def send_broadcast(self, code, payload=None, ttl=4):
        self._broadcast_messages.set_message(code, ttl, payload)
        if self.network_delegate != None:
            self.network_delegate.broadcast_messages(self._broadcast_messages)

    @property
    def name(self):
        return self._name

    def add_observer(self, observer):
        if observer in self._observers:
            return
        self._observers.append(observer)

    def remove_observer(self, observer):
        if observer in self._observers:
            self._observers.remove(observer)

    def _decode_properties(self, properties):
        attributes = {}
        messages = []

        for property in properties:
            first_broadcast_code_id = 0x90
            property_code = property[0]
            if property_code < first_broadcast_code_id:
                attr = LampAttribute.decode(property)
                attributes[attr.code] = attr
            else:
                messages.append(BroadcastMessage.decode(property))

        return attributes, messages

    async def observed_lamp(self, id, name, rssi, raw_properties):
        attributes, messages = self._decode_properties(raw_properties)

        is_new_lamp = not id in self.model.lamps
        changed_lamp_attributes = self.model.observed_lamp(id, name, rssi, attributes)

        messages_changed = self._broadcast_messages.update_messages(messages)

        if is_new_lamp:
            lamp = self.model.lamps[id]
            await self._publish_new_lamp_appeared(lamp)
            await self._publish_lamp_attribute_changed(lamp, changed_lamp_attributes)
        elif changed_lamp_attributes != None:
            lamp = self.model.lamps[id]
            await self._publish_lamp_changed(lamp)
            await self._publish_lamp_attribute_changed(lamp, changed_lamp_attributes)

        if messages_changed:
            if self.network_delegate != None:
                self.network_delegate.broadcast_messages(self._broadcast_messages)
            await self._publish_messages_observed(self._broadcast_messages)

    async def _monitor_loop(self):
        while self._monitoring:
            departed_lamps = self.model.departed_lamps
            if len(departed_lamps) > 0:
                await self._publish_lamps_departed(departed_lamps)
                self.model.prune_stale_lamps()

            expired_messages = self._broadcast_messages.prune_expired_messages()
            if len(expired_messages) > 0:
                await self._publish_messages_stopped(expired_messages)

            await asyncio.sleep_ms(self.MONITOR_INTERVAL_MS)

    async def start_monitoring(self):
        self._monitoring = True
        self._monitor_task = asyncio.create_task(self._monitor_loop())

    async def stop_monitoring(self):
        if not self._monitor_task:
            return

        self._monitoring = False
        await self._monitor_task
        self._monitor_task = None

    async def _publish_new_lamp_appeared(self, new_lamp):
        for observer in self._observers:
            await observer.new_lamp_appeared(new_lamp)

    async def _publish_lamp_changed(self, lamp):
        for observer in self._observers:
            await observer.lamp_changed(lamp)

    async def _publish_lamp_attribute_changed(self, lamp, changed_attributes):
        for observer in self._observers:
            for attribute in changed_attributes:
                await observer.lamp_attribute_changed(lamp, lamp.attributes[attribute])

    async def _publish_lamps_departed(self, lamps):
        for observer in self._observers:
            await observer.lamps_departed(lamps)

    async def _publish_messages_observed(self, messages):
        for observer in self._observers:
            for message in messages.values():
                await observer.message_observed(message)

    async def _publish_messages_stopped(self, codes):
        for observer in self._observers:
            for code in codes:
                await observer.message_stopped(code)
