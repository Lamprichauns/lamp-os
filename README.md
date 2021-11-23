# LampOS 

## Hardware

This is being developed for the ESP32 

## Dev setup (for vscode)

- pip install -r requirements.txt
- If using asdf, run `asdf reshim`
- Install PyMakr extension in VSCode (use 1.1.13 - 1.1.14 is broken.)
- For ESP32 you don't need to do anything else, for the ESP8266 add `Silicon Labs` to the list of `autoconnect_comport_manufacturers` in the global PyMakr config.
- Now you can run the code on the arduino from within VScode, as well as REPL selected code, etc.

## Lamp creation

To create a lamp, create `src/lamps/lampname.py` using `src/main.py` as a starting point.

For now to install, modify main.py to point to the right lamp and upload everything in `src`.


## Flashing lamps

To flash a lamp to an arduino, run: `invoke flash [port] [lampname]` 

eg. `invoke flash /dev/tty.usbserial-D3071K6D gramp` 
