import network, re, uasyncio
from time import sleep
from machine import Timer
from simple_lamp import SimpleLamp

signal_threshold    = -70
lamp_scan_interval  = 10000

class Lamp(SimpleLamp):
    # SSID is "LampOS-lampname-basecolor-shadecolor"
    def broadcast(self):
        self.wifi_ap  = network.WLAN(network.AP_IF)

        ssid = "LampOS-%s-%s-%s" % (self.name, self.base_color, self.shade_color)
        self.wifi_ap.active(True)
        self.wifi_ap.config(essid=ssid, password="lamprichauns")

    async def scan(self):
        networks = self.wifi_sta.scan()
        nearby_lamps = []
        self.lamp_network["joined"].clear()
        self.lamp_network["left"].clear()

        print("\nScanning for lamps..")

        for ssid, bssid, channel, rssi, authmode, hidden in networks:
            ssid = ssid.decode("utf-8")

            match = re.search(r'LampOS-(\w+)-(\#\w+)-(\#\w+)', ssid)
            # Old debug, delete me soon.
            # if match: print("Match: %s (%s / %s)@ %d db" % (match.group(1), match.group(2), match.group(3), rssi))

            # cycle through all the found lamps
            if (match and rssi >= signal_threshold):
                found_lamp = SimpleLamp(match.group(1), match.group(2), match.group(3))
                nearby_lamps.append(found_lamp)

                # Track who has joined the network
                if not (found_lamp in self.lamp_network["current"]):
                    self.lamp_network["joined"].append(found_lamp)
                    print("A new lamp is nearby: %s[%s] (%d db)" % (found_lamp.name, found_lamp.lamp_id, rssi))


        # Track who has left the network
        for lamp in self.lamp_network["current"]:
            if not(lamp in nearby_lamps):
                self.lamp_network["left"].append(lamp)
                print("A lamp has left: %s[%s]" % (lamp.name, lamp.lamp_id))

        # Set current list
        self.lamp_network["current"] = nearby_lamps.copy()

    def wake(self):
        self.broadcast()
        print("%s is awake!" % (self.name))

        self.wifi_sta = network.WLAN(network.STA_IF)
        self.wifi_sta.active(True)
        self.lamp_network = {"current": [], "joined": [], "left": []}

        timer = Timer(-1)
        timer.init(period=5000, mode=Timer.PERIODIC, callback=lambda t:uasyncio.run(self.scan()))

        while True:
            if not self.callback == None: self.callback()
            sleep(0.25)        

    # Register callback for lamp behaviours. 
    def register_callback(self, l): 
            self.callback = l

