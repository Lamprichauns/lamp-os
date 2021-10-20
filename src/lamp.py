from machine import Pin
from machine import Timer
from machine import Pin
from time import sleep
import network, re, _thread
import binascii, hashlib

# Signal strength threshold - weaker signal than this doesn't count as "close".
signal_threshold    = -70
lamp_scan_interval  = 10000

class Lamp:
    def __init__(self, name, base_color, shade_color):
        self.name = name
        self.base_color = base_color
        self.shade_color = shade_color

        attrs = f"{self.name}-{self.base_color}-{self.shade_color}"
        sha = hashlib.sha1(attrs)
        self.lamp_id = binascii.hexlify(sha.digest()).decode()

    def __eq__(self,other):
        return isinstance(other, Lamp) and self.lamp_id == other.lamp_id

# Scan networks and find lamps, and track lamps who
def scan_networks():
    networks = wifi_sta.scan()
    nearby_lamps = []
    lamp_network["joined"].clear()
    lamp_network["left"].clear()

    print("\nScanning for lamps..")

    for ssid, bssid, channel, rssi, authmode, hidden in networks:
        ssid = ssid.decode("utf-8")

        match = re.search(r'LampOS-(\w+)-(\#\w+)-(\#\w+)', ssid)
         # Old debug, delete me soon.
        # if match: print("Match: %s (%s / %s)@ %d db" % (match.group(1), match.group(2), match.group(3), rssi))

        # cycle through all the found lamps
        if (match and rssi >= signal_threshold):
            found_lamp = Lamp(match.group(1), match.group(2), match.group(3))
            nearby_lamps.append(found_lamp)

            # Track who has joined the network
            if not (found_lamp in lamp_network["current"]):
                lamp_network["joined"].append(found_lamp)
                print("A new lamp is nearby: %s[%s] (%d db)" % (found_lamp.name, found_lamp.lamp_id, rssi))


    # Track who has left the network
    for lamp in lamp_network["current"]:
        if not(lamp in nearby_lamps):
            lamp_network["left"].append(lamp)
            print("A lamp has left: %s[%s]" % (lamp.name, lamp.lamp_id))

    # Set current list
    lamp_network["current"] = nearby_lamps.copy()

# Change the color of the shade and base if needed
#def adjust_colors
  # Call callbacks base_color(), shade_color() to determine color, use config if callbacks are not defined.
  # if new color is not the same as last set color, set the new colors


# Setup this lamp upon boot.
def setup():
    global lamp_network, wifi_sta, config

    # Default lamp settings. White/White and no name
    config = Lamp("noname", "#ffffff", "#ffffff")
    lamp_network = {"current": [], "joined": [], "left": []}

    # Init wifi
    wifi_sta = network.WLAN(network.STA_IF)
    wifi_sta.active(True)

def broadcast():
    wifi_ap  = network.WLAN(network.AP_IF)
    # SSID is "LampOS-lampname-basecolor-shadecolor"
    ssid = "LampOS-%s-%s-%s" % (config.name, config.base_color, config.shade_color)
    wifi_ap.active(True)
    wifi_ap.config(essid=ssid, password="lamprichauns")

# Scan for lamps.
# Making this too frequent could also result in unfavourably reactivity if a lamp
# is on the edge and ebbing in and out of "nearness" - and scanning takes extra power!
# Using virtual timer instead of hardware for compatibility with ESP8266.
def run():
    broadcast()
    print("%s is awake!" % (config.name))

    led = Pin(5,Pin.OUT)

    timer = Timer(-1)
    timer.init(period=5000, mode=Timer.PERIODIC, callback=lambda t:_thread.start_new_thread(scan_networks,()))

    # Loop pretty quickly, so we can do sub-second color changes if we want.
    while True:
        led.value(not led.value())
        # adjust_colors()
        sleep(0.25)
