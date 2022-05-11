import network

# Creates a simple wifi access point to eventually use with sockets
class AccessPoint():
    def __init__(self, ssid, password):
        print("Starting Access Point")
        self.access_point = network.WLAN(network.AP_IF)
        self.access_point.active(True)
        self.access_point.config(essid=ssid, password=password)
        while self.access_point.active() is False:
            pass

        print(self.access_point.ifconfig())
