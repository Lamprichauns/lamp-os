import network, re, uasyncio
from time import sleep
from machine import Timer
from simple_lamp import SimpleLamp
import neopixel, machine

signal_threshold    = -70
lamp_scan_interval  = 10000

base_led_config     = { "pin": 5, "leds": 5 }
shade_led_config    = { "pin": 4, "leds": 5 }

class Lamp(SimpleLamp):
    # Broadcast that this lamp is here
    # SSID is "LampOS-lampname-basecolor-shadecolor"
    def broadcast(self):
        self.wifi_ap  = network.WLAN(network.AP_IF)

        ssid = "LampOS-%s-%s-%s" % (self.name, self.base_color, self.shade_color)
        self.wifi_ap.active(True)
        self.wifi_ap.config(essid=ssid, password="lamprichauns")

    # Scan for other lamps
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

    # Register callback for lamp behaviours.
    def register_callback(self, l):
            self.callback = l

    # Set the shade and base to fixed color based on the config for this lamp.
    def reset_lights(self):
        self.adjust_shade(self.shade_color)
        self.adjust_base(self.base_color)


    # Adjust colors of a led strand. There are convenience functions for shade and base below
    def adjust_leds(self, strand, colors):
        if strand in ["shade","base"]:
            led_count = base_led_config["leds"] if strand == "base" else shade_led_config["leds"]

            if isinstance(colors, str):
                colors = build_color_array(hex_to_rgb(colors), led_count)

            if not colors == self.current_colors[strand]:
                print("changing %s colors" % strand)
                write_led_colors(pixels[strand], colors)
                self.current_colors[strand] = colors

    # Adjust base colors to an array of RGBW tuples. This should be exactly the right number of colors for the channel.
    def adjust_base(self, colors):
        self.adjust_leds("base", colors)

    # Adjust the shade color to an array of RGBW tuples. This should be exactly the right number of colors for the channel.
    def adjust_shade(self, colors):
        self.adjust_leds("shade", colors)

    # Wake up this lamp!
    def wake(self):
        self.current_colors = {  "base": "", "shade": ""}

        self.broadcast()
        self.reset_lights()

        print("%s is awake!" % (self.name))

        self.current_colors = {  "base": "", "shade": ""}

        self.wifi_sta = network.WLAN(network.STA_IF)
        self.wifi_sta.active(True)
        self.lamp_network = {"current": [], "joined": [], "left": []}

        timer = Timer(-1)
        timer.init(period=5000, mode=Timer.PERIODIC, callback=lambda t:uasyncio.run(self.scan()))

        while True:
            if not self.callback == None: self.callback()
            sleep(0.25)        

# Util functions for color stuff.

pixels = {
    "shade": neopixel.NeoPixel(machine.Pin(shade_led_config["pin"]), shade_led_config["leds"], bpp=4),
    "base": neopixel.NeoPixel(machine.Pin(base_led_config["pin"]), base_led_config["leds"], bpp=4)
}

# Write  colors to leds on the right pin. Colors should be an array of tuples (R,G,B,W)
def write_led_colors(led_channel, colors):
    for i, c in enumerate(colors):
        led_channel[i] = c
        led_channel.write()


def build_color_array(value, length):
    colors = []
    for i in range(length):
        colors.append(value)
    return colors

# Convert hex colors to RGBW - Automatically flip full white to 0,0,0,255 (turn on warm white led
# instead of each individual color)
def hex_to_rgb(value):
    value = value.lstrip('#')
    rgb = tuple(int(value[i:i+2], 16) for i in (0, 2, 4))
    return (0,0,0,255) if rgb == (255,255,255) else rgb + (0,)