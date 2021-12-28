# LampOS 

## Hardware

This is being developed for the ESP32 

## Dev setup (for vscode)

- pip install -r requirements.txt
- If using asdf, run `asdf reshim`
- Install PyMakr extension in VSCode (use 1.1.13 - 1.1.14 is broken.)
- Now you can run the code on the arduino from within VScode, as well as REPL selected code, etc.


## Lamp creation

To create a lamp, create `src/lamps/lampname.py` using `src/main.py` as a starting point.

For now to install, modify main.py to point to the right lamp and upload everything in `src`.

Wiring and assembly info will be here later.

### Lamp Personality & Behaviours 

The vision for the lamps is that they remain mostly still and static, as a contrast to the plethora of sound reactive and blinky light art out there. The brightness of the lamps and the colorful glowing base draws attention to the juxtaposition of an ordinary household object in extraordinary places. 

Within this vision there is room for the lamps to have personality, shown through subtle behaviour based on things like time, randomness, presence/absence of other lamps, etc. 

With these sorts of subtle changes this also introduces an element where people may begin to realize things are not as static as they seem, creating a somewhat complex puzzle for people to solve and talk about.

### Implementation  

In order to implement this sort of behaviour each lamp has it's own relatively simple python file that configures the name, base color, and shade color of the lamp, as well as providing the ability for a callback to be be registered. 

The lamps are aware of how many other lamps are nearby, as well as their names and configured base and shade colors. These can be queried within the callback to control behaviour. 

Some examples of things that can be done with this: 

  - A lamp could have a list of lamps it considers friends and ones it doesn't like as much. It could change it's color based on the presence of or ratio of friends to other lamps.
  - A lamp could simply be smitten with another lamp, and it could blush by a section of it's shade shifting to a redish color when the other lamp is near.
  - A lamp could be shy, and get much dimmer when too many other lamps are around. 
  - A lamp could be afraid of a specific lamp and flicker erratically when that lamp initially shows up, and then going more dim while it's around.
  - A lamp could be a trickster and simply at random switch colors for very brief (half seconds) periods. Maybe occasionaly drift it's colors over the course of the night. 
  - A lamp could steal the base color of the most recent lamp that has showed up.
  - A lamp could collect base colors of nearby lamps and comprise it's base color as a stacked rainbow of those colors instead of a solid color
  
There will be info here later on how to implement this, as well as contact info in case you want us to help you do it.


## Flashing lamps

To flash a lamp to an arduino, run: `invoke flash [port] [lampname]` 

eg. `invoke flash /dev/tty.usbserial-D3071K6D gramp` 




--- 
t = time.ticks_us() 

delta = time.ticks_diff(time.ticks_us(), t)
print('step- Time = {:6.3f}ms'.format( delta/1000))  
