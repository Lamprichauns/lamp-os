# LampOS

## Hardware

This is being developed for the ESP32

## Software Prerequisites

### Install Python 3.x and Pip on your machine

<https://www.python.org/downloads>

### Install Micropython on your ESP32

To setup the ESP32 You'll need to flash a fresh Micropython on the board. First you'll need esptool:

```bash
pip install esptool
```

Next download the latest micropython binary from: <https://micropython.org/download/esp8266/>

Finally erase and flash the board with (with the ESP32 Thing you need to hold reset down at the start of the flash):

```bash
esptool.py --port /dev/tty.usbserial-0246D45F erase_flash
esptool.py --chip esp8266 --port /dev/tty.usbserial-0246D45F --baud 460800 write_flash --flash_size=detect -fm dout 0 esp8266-20210902-v1.17.bin

```

## Dev setup (for Atom)

- pip install -r requirements.txt
- Install PyMakr extension in Atom IDE
- Now you can run the code on the arduino from within Atom, as well as REPL selected code, etc.

> Note: VSCode is very temperamental with connecting to these devices at the moment

## Loading Data on the ESP32s

### Atom IDE Usage

Plug your hardware into a USB port

Navigate to Packages > PyMakr > PyCom Console

In the top left tab of your console, choose your project folder eg. src

In the connection tab, choose your USB or COM port and click connect

The left side bar will go green once connected. Use the buttons on the left side to upload your code to the onboard ESP32

### Commandline Usage

The lamp loading mechanism works by utilizing the lamps filename as the lamp to load.  All `invoke` commands (or `inv` for short) that interact with the board should be followed by the file of the lamp in question.  eg: `inv run twinkle`.

#### Flash and run the lamp on the target device

```bash
inv run gramp --port /dev/tty.usbserial-0246D45F
```

#### Flash the lamp to device

Note that flashing is a progressive operation. The build system will try to determine what files that need to be updated and only upload changed files since the last flash. If there is issues run `inv wipe --port [device]` first.

```bash
inv flash gramp --port /dev/tty.usbserial-0246D45F
```

#### Clean off all the code from

```bash
inv wipe --port /dev/tty.usbserial-0246D45F
```

#### Run Test Suite

```bash
inv test
```

#### Clean interim build files

```bash
inv clean
```

## Lamp creation

To create a lamp, create `src/lamps/lampname.py`.

Reference your lamp by making a copy of `src/main_sample.py` to `src/main.py` and update the reference to your lamp name

Have a look at other examples of lamps to get an idea of the capabilities of LampOS

## Application Structure

`src/` -  is where all the code for a lamp lives, this gets flashed to devices (in entirety, so any lamp could really switch to another)
`test/` -  is test cases, pretty much just the network module. Testing the app entry point and BLE layer is possible, but this was already getting large.
`src/components` - The hardware definitions, drivers and synchronous initialization items are classified as components
`src/behaviours` - Asynchronous tasks that run and give the lamp unique features, long running tasks and puzzles
`src/lamp_core` - Core code that can be leveraged for all lamps.
`src/utils` - Utility files not useful for all lamps, but handy to have around
`src/vendor` - Any borrowed python code from the community goes here. Please make sure to retain a reference to github in the entry point file
`src/lamps` - Each lamp.  Each lamp will have it's own entry which is loaded from `main.py` the files live here.

## Lamp Personality & Behaviours

The vision for the lamps is that they remain mostly still and static, as a contrast to the plethora of sound reactive and blinky light art out there. The brightness of the lamps and the colorful glowing base draws attention to the juxtaposition of an ordinary household object in extraordinary places.

Within this vision there is room for the lamps to have personality, shown through subtle behaviour based on things like time, randomness, presence/absence of other lamps, etc.

With these sorts of subtle changes this also introduces an element where people may begin to realize things are not as static as they seem, creating a somewhat complex puzzle for people to solve and talk about.

## Implementation  

In order to implement this sort of behaviour each lamp has it's own relatively simple python file that configures the name, base color, and shade color of the lamp.

The lamps are aware of how many other lamps are nearby, as well as their names and configured base and shade colors.

Some examples of things that can be done with this:

- A lamp could have a list of lamps it considers friends and ones it doesn't like as much. It could change it's color based on the presence of or ratio of friends to other lamps.
- A lamp could simply be smitten with another lamp, and it could blush by a section of it's shade shifting to a reddish color when the other lamp is near.
- A lamp could be shy, and get much dimmer when too many other lamps are around.
- A lamp could be afraid of a specific lamp and flicker erratically when that lamp initially shows up, and then going more dim while it's around.
- A lamp could be a trickster and simply at random switch colors for very brief (half seconds) periods. Maybe occasionally drift its colors over the course of the night.
- A lamp could steal the base color of the most recent lamp that has showed up.
- A lamp could collect base colors of nearby lamps and comprise its base color as a stacked rainbow of those colors instead of a solid color
