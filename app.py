from machine import Pin
from time import sleep
import network
import re

# Lamp Config

lamp_name   = "vamp"
color       = {"base": (0, 200, 0, 31), "shade": (255, 255, 255, 31)}

##########################

def scan_networks():
    networks = wifi_sta.scan()
    nearby_lamps = []
    lamp_network["joined"].clear()
    lamp_network["left"].clear()
    
    for ssid, bssid, channel, rssi, authmode, hidden in sorted(networks, key=lambda x: x[3], reverse=True):
        ssid = ssid.decode("utf-8")
        
        match = re.search(r'LampOS-(\w+)', ssid)
        
        # cycle through all the found lamps
        if (match):
            found_lamp = match.group(1)
            nearby_lamps.append(found_lamp)
            
            # Track who's joined the network
            if not (found_lamp in lamp_network["current"]):
                lamp_network["joined"].append(found_lamp)
                print("A new lamp is nearby: %s, (%d db)" % (match.group(1), rssi))                
        
        # Track who's left the network
        for lamp in lamp_network["current"]:
            if not(lamp in nearby_lamps):
                lamp_network["left"].append(lamp)
                print("A lamp has left: %s" % (lamp))                
        
        # Set current list
        lamp_network["current"] = nearby_lamps.copy()


def adjust_colors():
    print("doesnt work yet")


lamp_network = {"current": [], "joined": [], "left": []}

# Init wifi
wifi_sta = network.WLAN(network.STA_IF)
wifi_ap  = network.WLAN(network.AP_IF)

wifi_ap.active(True)
wifi_sta.active(True)

ssid = "LampOS-%s" % (lamp_name)
wifi_ap.config(essid=ssid, password="lamprichauns")

print("%s is awake!" % (lamp_name))

while True:
    scan_networks()
    # adjust_colors()
    sleep(0.5)
