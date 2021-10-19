# LampOS 

## Dev setup (for vscode)

- Install micropy-cli
- Install PyMakr extension in VSCode
- That's it! Then you can run the code on the arduino from within VScode, as well as REPL selected code, etc.


## Lamp creation

To create a lamp, create `src/lamps/lampname.py` using `src/main.py` as a starting point.

For now to install, modify main.py to point to the right lamp and upload everything in `src`.


# Todo list:
 - When using RGBwW strips, automatically use the white when the color is set to #ffffff
 - include the configured base and shade colors in the SSID so other lamps can know what colors their friends are
 - make a lampnpm install serial port object for the lamp_network arrays instead of the strings of lamp names, so we can include default colors in that
 - Create hooks from color setting and move this all to base.py, so we can have [lampname].py that just has config and callbacks to handle behaviour for individual lamps
 - To avoid uploading extra files and keep things simpler, create a build script that: 
    - takes the name of the lamp
    - uploads boot.py and lamp.py
    - uploads lamps/name.py as `main.py`, but with a new line containing `lamp.run()` to the end of the file
    - Leave the main.py as is though, for development/testing different lamps
