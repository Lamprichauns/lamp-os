from machine import Pin
from machine import Timer
from machine import Pin
from time import sleep
import network
import re
import _thread

# Signal strength threshold - weaker signal than this doesn't count as "close".
signal_threshold    = -70
lamp_scan_interval  = 10000

class Lamp:
    def __init__(self, name, base_color, shade_color):
        self.name = name
        self.base_color = base_color
        self.shade_color = shade_color

# Scan networks and find lamps, and track lamps who
def scan_networks():
    networks = wifi_sta.scan()
    nearby_lamps = []
    lamp_network["joined"].clear()
    lamp_network["left"].clear()

    print("\nScanning for lamps..")

    for ssid, bssid, channel, rssi, authmode, hidden in networks:
        ssid = ssid.decode("utf-8")

        match = re.search(r'LampOS-(\w+)', ssid)
        if match: print("Match: %s @ %d db" % (match.group(1), rssi))

        # cycle through all the found lamps
        if (match and rssi >= signal_threshold):
            found_lamp = match.group(1)
            nearby_lamps.append(found_lamp)

            # Track who has joined the network
            if not (found_lamp in lamp_network["current"]):
                lamp_network["joined"].append(found_lamp)
                print("A new lamp is nearby: %s, (%d db)" % (match.group(1), rssi))


    # Track who has left the network
    for lamp in lamp_network["current"]:
        if not(lamp in nearby_lamps):
            lamp_network["left"].append(lamp)
            print("A lamp has left: %s" % (lamp))

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
    wifi_ap  = network.WLAN(network.AP_IF)

    wifi_ap.active(True)
    wifi_sta.active(True)

    # TODO: make this "LampOS-lampname-basecolor-shadecolor"
    ssid = "LampOS-%s" % (config.name)
    wifi_ap.config(essid=ssid, password="lamprichauns")
   

# Scan for lamps.
# Making this too frequent could also result in unfavourably reactivity if a lamp
# is on the edge and ebbing in and out of "nearness" - and scanning takes extra power!
# Using virtual timer instead of hardware for compatibility with ESP8266.
def run():
    print("%s is awake!" % (config.name))

    led = Pin(5,Pin.OUT)

    timer = Timer(-1)
    timer.init(period=5000, mode=Timer.PERIODIC, callback=lambda t:_thread.start_new_thread(scan_networks,()))

    # Loop pretty quickly, so we can do sub-second color changes if we want.
    while True:
        led.value(not led.value())
        # adjust_colors()
        sleep(0.25)
