# LampOS 

## Hardware

This is being developed for the ESP32 

## Dev setup (for vscode)

- pip install -r requirements.txt
- If using asdf, run `asdf reshim`
- Install PyMakr extension in VSCode (use 1.1.13 - 1.1.14 is broken.)
- Now you can run the code on the arduino from within VScode, as well as REPL selected code, etc.

## Lamp creation

To create a lamp, create `src/app/lamps/lampname.py`.

Wiring and assembly info will be here later.

## Loading Data on the ESP32s

The lamp loading mechanism works by utilizing the lamps filename as the lamp to load.  All `invoke` commands (or `inv` for short) that interact with the board should be followed by the file of the lamp in question.  eg: `inv run twinkle`.

### `lamp-mapping.json`

The `lamp-mapping.json` file allows to set a default pairing of lamps to devices so the `--port` argument can be dropped.

### Command List:

#### Flash and run the lamp on the target device
```
inv run gramp --port /dev/tty.usbserial-0246D45F
```

#### Flash the lamp to device

Note that flashing is a progressive operation. The build system will try to determine what files that need to be updated and only upload changed files since the last flash. If there is issues run `inv wipe --port [device]` first.
```
inv flash gramp --port /dev/tty.usbserial-0246D45F
```

#### Clean off all the code from 
```
inv wipe --port /dev/tty.usbserial-0246D45F
```

#### Run Test Suite
```
inv test
```

#### Clean interim build files
```
inv clean
```

## Interacting with Other Lamps

One of the interesting things about these lamps is they are aware of each other.  Each lamp that is created is handed a networking object that allows you to share details about your lamp with the network, or recieve information about other lamps.

#### Announcing Details About Yourself
```
network.announce_attribute(Code.BASE_COLOR, (255, 200, 0, 0))
```

#### Broadcasting Messages
Any lamp that recieves a broadcast message will replay the message to the other lamps.  These messages have a `ttl` that states how long they will last. Each messages recieved by other lamps has it's `ttl` decremented by `1` to ensure that the messages don't get trapped in the network for ever. `ttl` is typically seconds.

```
network.broadcast_message(Code.BASE_OVERRIDE, 4, (255, 0, 0, 0))
```

#### Listen For Network Changes

```
from ..lamp_core.base_lamp import BaseLamp
from ..network.network import LampNetworkObserver

class MyLamp(BaseLamp, LampNetworkObserver):
    __init__(self, network):
        network.attach_observer(self)

    # Each of these delegate methods can be optionally added to recieve
    # information about the various network activites.
    async def new_lamp_appeared(self, new_lamp):
        pass

    async def lamp_changed(self, lamp):
        pass

    async def lamp_attribute_changed(self, lamp, attribute):
        pass

    async def lamps_departed(self, lamps):
        pass

    async def message_observed(self, message):
        pass

    async def message_stopped(self, code):
        pass

```

### Application Structure

`src/` -  is where all the code for a lamp lives, this gets flashed to devices (in entirety, so any lamp could really switch to another)
`test/` -  is test cases, pretty much just the network module. Testing the app entry point and BLE layer is possible, but this was already getting large.
`src/*.py` - Just the bare bones boot files for the app, rest should live in the app folder. Makes things more portable
`src/app/network` - The networking module.
`src/app/lamp_core` - Core code that can be leveraged for all lamps. I think can do more house keeping here.
`src/app/utils` - Utility files not useful for all lamps, but handy to have around
`src/app/lamps` - Each lamp.  Each lamp will have it's own entry which is loaded from `app.py` the files live here.

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

## Firmware Flashing

To setup the ESP32 You'll need to flash a fresh Micropython on the board. First you'll need esptool:
```pip install esptool```

Next download the latest micropython binary from: https://micropython.org/download/esp8266/

Finally erase and flash the board with (with the ESP32 Thing you need to hold reset down at the start of the flash):

```
esptool.py --port /dev/tty.usbserial-0246D45F erase_flash
esptool.py --chip esp8266 --port /dev/tty.usbserial-0246D45F --baud 460800 write_flash --flash_size=detect -fm dout 0 esp8266-20210902-v1.17.bin

```

--- 


### Curious?
```
t = time.ticks_us() 

delta = time.ticks_diff(time.ticks_us(), t)
print('step- Time = {:6.3f}ms'.format( delta/1000))  
```