from machine import Pin
from machine import Timer
from time import sleep
import network
import re

# Lamp Config

lamp_name   = "womp"
color       = {"base": '#00ff00', "shade": '#ffffff'}

# Signal strength threshold - weaker signal than this doesn't count as "close".
signal_threshold = -90

# todo list:
# - When using RGBwW strips, automatically use the white when the color is set to #ffffff
# - include the configured base and shade colors in the SSID so other lamps can know what colors their friends are
# - make a lamp object for the lamp_network arrays instead of the strings of lamp names, so we can include default colors in that
# - Create hooks from color setting and move this all to base.py, so we can have [lampname].py that just has config and callbacks to handle behaviour for individual lamps

##########################


class Lamp:
    def __init__(self, name, base_color, shade_color):
        self.name = name
        self.base_color = base_color
        self.shade_color = shade_color

def setup():
    global lamp_network, wifi_sta

    lamp_network = {"current": [], "joined": [], "left": []}

    # Init wifi
    wifi_sta = network.WLAN(network.STA_IF)
    wifi_ap  = network.WLAN(network.AP_IF)

    wifi_ap.active(True)
    wifi_sta.active(True)

    # TODO: make this "LampOS-lampname-basecolor-shadecolor"
    ssid = "LampOS-%s" % (lamp_name)
    wifi_ap.config(essid=ssid, password="lamprichauns")

    print("%s is awake!" % (lamp_name))

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


#def adjust_colors
  # Call callbacks base_color(), shade_color() to determine color, use config if callbacks are not defined.
  # if new color is not the same as last set color, set the new colors

#####

setup()

# Scan for lamps.
# Making this too frequent could also result in unfavourably reactivity if a lamp
# is on the edge and ebbing in and out of "nearness" - and scanning takes extra power!
# Using virtual timer instead of hardware for compatibility with ESP8266.
timer = Timer(-1)
timer.init(period=10000, mode=Timer.PERIODIC, callback=lambda t:scan_networks())

# Loop pretty quickly, so we can do sub-second color changes if we want.
while True:
    # adjust_colors()
    sleep(0.25)
