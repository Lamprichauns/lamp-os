from machine import Pin
from time import sleep
import network

# Lamp Config
color = {"base": (200, 150, 0, 31), "shade": (255, 255, 255, 31)}


led = Pin(2, Pin.OUT)

wlan = network.WLAN(network.STA_IF)
wlan.active(True)


def scan_networks():
    networks = wlan.scan()
    
    for ssid, bssid, channel, rssi, authmode, hidden in sorted(networks, key=lambda x: x[3], reverse=True):
        ssid = ssid.decode("utf-8")
        print("ssid: %s chan: %d rssi: %d  " % (ssid, channel, rssi))

    # Let's figure out and store a list of nearby lamps containing name & relative distance
    # let's store previous network size and current network size so we can compare

# def adjust_colors():


while True:
    # Blinky blink just so for now we can see things are working
    led.value(
        not led.value()
    ) 
   
    scan_networks()

    # adjust_colors()

    sleep(0.5)
