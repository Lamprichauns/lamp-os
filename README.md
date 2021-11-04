# LampOS 

## Hardware

This has been tested on the ESP32 and ESP8266. 

You need to first flash the arduino with micropython firmware (for ESP32: https://docs.micropython.org/en/latest/esp32/tutorial/intro.html)
 

## Dev setup (for vscode)

- pip install -r requirements.txt
- If using asdf, run `asdf reshim`
- Install PyMakr extension in VSCode (use 1.1.13 - 1.1.14 is broken.)
- For ESP32 you don't need to do anything else, for the ESP8266 add `Silicon Labs` to the list of `autoconnect_comport_manufacturers` in the global PyMakr config.
- Now you can run the code on the arduino from within VScode, as well as REPL selected code, etc.

## Notes

 - To support slower boards like the ESP8266 we need to avoid some things: 
    - dynamic string interpolation (f"foo {bar}")
    - threads (uasync works)

## Lamp creation

To create a lamp, create `src/lamps/lampname.py` using `src/main.py` as a starting point.

For now to install, modify main.py to point to the right lamp and upload everything in `src`.


## Flashing lamps

To flash a lamp to an arduino, run: `invoke flash [port] [lampname]` 

eg. `invoke flash /dev/tty.usbserial-D3071K6D gramp` 

# Todo list:
 - When using RGBwW strips, automatically use the white when the color is set to #ffffff
 - Create hooks from color setting and move this all to base.py, so we can have [lampname].py that just has config and callbacks to handle behaviour for individual lamps





 Things to interrogate about other Lamps:

   lamp.lamp_network["current"]     : current lamps nearby
   lamp.lamp_network["joined"]      : lamps that just got here (since the last scan)
   lamp.lamp_network["left"]        : lamps that just left (since the last scan)

 These are arrays of Lamp objects, which implement:

   lamp.name         : The name of the lamp
   lamp.base_color   : the original/config'd base color of the lamp
   lamp.shade_color  : the original/config'd shade color of the lamp

 Other interesting things:

   shade_led_config["leds"]  : number of leds in the shade channel
   base_led_config["leds"]   : number of leds in the base channel
   lamp                      : The current lamp's Lamp object.
