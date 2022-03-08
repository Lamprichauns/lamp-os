import json
import uasyncio as asyncio
from .network.coding import Codes
from .network.network import LampNetwork
from .network.lamp_bluetooth import LampBluetooth

class App:
    '''
    Main entry point for App. Handles dynmically loading lamps from config.json
    '''

    CONFIG_FILE = "/config.json"

    def __init__(self):
        self.lamp = None

        self._load_config()

        self.network = LampNetwork(self.config['name'])
        self._ble = LampBluetooth(self.network)

        self.network.announce_attribute(Codes.VERSION, int(self.config['version']))

        self._lamp_class = None
        App.shared_app = self
        self._load_lamp_module()

        if self._lamp_class is not None:
            new_lamp = self._lamp_class(self.network)
            self.lamp = new_lamp

    def _load_config(self):
        with open(self.CONFIG_FILE) as file:
            self.config = json.load(file)

        if self.config['name'] is None:
            raise Exception('No lamp name provided!')

        if self.config['module'] is None:
            raise Exception('No lamp source name provided!')

    def _load_leaf_module(self, module_string):
        module = __import__(module_string)
        parts = module_string.split('.')
        parts = parts[1:]
        for part in parts:
            module = getattr(module, part)

        return module

    def _load_lamp_module(self):
        self._lamp_module = self._load_leaf_module(self.config['module'])

        try:
            self._lamp_class = getattr(self._lamp_module, self.config['name'])
            print(f"Loaded Lamp: {self.config['name']} from {self.config['module']}")
        except AttributeError:
            print(f"Loaded Lamp Module: {self.config['module']}")

    def wake(self):
        '''Entry point for app main coroutine'''
        asyncio.run(self._main())

    async def _main(self):
        await self._ble.enable()
        await self.network.start_monitoring()

        while True:
            if self.lamp:
                await self.lamp.update()

            await asyncio.sleep_ms(10)
