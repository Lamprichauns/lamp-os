from machine import Pin
from time import sleep
import network
import re

# Lamp Config

lamp_name   = "gramp"
color       = {"base": (0, 200, 0, 31), "shade": (255, 255, 255, 31)}

##########################

# Adjust methods should by default adjust to the configured colors.. 
# any behavioural changes can live within these methods

# Currently things to look at: 
#   - lamp_network['current']  - An array of the names current lamps nearby. 
#   - lamp_network['joined']  - An array of the names of lamps that just arrived (this will only be available for one loop, when they first arrive)
#   - lamp_network['left']  - An array of the names of lamps that just left (this will only be available for one loop, when they first leave).


#def adjust_base():
  
    
#def adjust_shade():

    
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
