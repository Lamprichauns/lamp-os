from machine import Pin
from time import sleep
import network
import re

# Lamp Config

lamp_name   = "gramp"
color       = {"base": '#00ff00', "shade": '#ffffff'}


# todo list: 
# - When using RGBwW strips, automatically use the white when the color is set to #ffffff
# - include the configured base and shade colors in the SSID so other lamps can know what colors their friends are
# - make a lamp object for the lamp_network arrays instead of the strings of lamp names, so we can include more metadata in that (colors, signal strength)
# - Create a faster loop 250ms or so, but ensure we only scan the network every 10 seconds or so

##########################

# Adjust methods should by default adjust to the configured colors. 
# Behavioural changes can live within these methods.

# Currently things to look at: 
#   - lamp_network['current']  - An array of the names current lamps nearby. 
#   - lamp_network['joined']  - An array of the names of lamps that just arrived (this will only be available for one loop, when they first arrive)
#   - lamp_network['left']  - An array of the names of lamps that just left (this will only be available for one loop, when they first leave).


#def adjust_base():
  # stuff to determine color...
  # set_color('base', color, brightness)
    
#def adjust_shade():
  # stuff to determine color...
  # set_color('base', color, brightness)

#def set_color(location, color)
  # Check current color & brightness of location and do nothing if the color is not changing, otherwise assign color.
  # Do this by assigning the current color and brightness to a global variable rather than querying the led strip, 
  # this will likely be called every 250ms.
    
def setup():
    global lamp_network, wifi_sta
    
    lamp_network = {"current": [], "joined": [], "left": []}
    
    # Init wifi
    wifi_sta = network.WLAN(network.STA_IF)
    wifi_ap  = network.WLAN(network.AP_IF)

    wifi_ap.active(True)
    wifi_sta.active(True)

    ssid = "LampOS-%s" % (lamp_name)
    wifi_ap.config(essid=ssid, password="lamprichauns")

    print("%s is awake!" % (lamp_name))

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


##### 

setup()

while True:
    scan_networks()
    # adjust_base()
    # adjust_shade()
    sleep(0.5)
