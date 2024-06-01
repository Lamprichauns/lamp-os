# LampOS

A platform for retrofitting traditional desk lamps with programmable LED controllers to build unique lighted art structures. Using standardized and user friendly hardware based on the ubiquitous ESP32, Neopixels and common sensors, this project seeks to simplify the job of building curious and surreal LED projects for the community to enjoy.

## Lamp Personality & Behaviours

The vision for the lamps is that they remain mostly still and static, as a contrast to the plethora of sound reactive and blinky light art out there. The brightness of the lamps and the colorful glowing base draws attention to the juxtaposition of an ordinary household object in extraordinary places.

Within this vision there is room for the lamps to have personality, shown through subtle behaviour based on things like time, randomness, sensor data, presence/absence of other lamps, etc.

With these sorts of subtle changes, people may begin to realize things are not as static as they seem, creating a somewhat complex puzzle for people to solve and talk about.

## Lamp Hardware Requirements

This software is intended for the ESP32 platform. Our preferred dev board is an ESP32-WROOM32 30 Pin board variant measuring no wider than 28mm with a chip antenna. Unsoldered/unwelded pins are preferred if possible. This space requirement is so the board can fit comfortably into a standard lamp socket. The boards can be had easily from Amazon and AliExpress for $5-10. The model we use has this pinout <https://lastminuteengineers.com/esp32-pinout-reference/>

By default, a lamp will use about 80 LEDs. The limiting factor at the moment is current draw. generally over 100 LEDs you may have stability issues with a conventional USB source. We recommend purchasing LEDs strips with the following specs:

- Around 2m in length
- SK6812 chipset
- IP67 waterproof
- 5VDC
- RGBWW (warm white) LED Strips
- Spacing of 60 LEDs/m

The motion sensor used in our examples is an MPU-6050 development board

A 10Ah battery pack with USB will run this device portably for around 7 hours

## Software Prerequisites

<https://www.python.org/downloads>
<https://nodejs.org/en/>
<https://atom.io>
<https://git-scm.com/downloads>

Once these are installed, reboot your machine to update all the paths. If they don't update, please manually add node, python and pip manually to your environment  

### Install Micropython on your ESP32

To setup the ESP32 You'll need to flash a fresh Micropython on the board. First you'll need esptool and invoke

```bash
pip install esptool invoke
```

To setup a new board, run:

```inv setup PORT```

Seperate tasks are also available:

```inv erase PORT
inv erase PORT
inv flash PORT
```

To upload all the code to the device:

```
inv upload PORT
```

and to update a specific lamp (the files in `src/lamps`) you can:

```
inv update PORT LAMP
```

You can also do this manually following the instructions here:

| Chip | Download |
| --- | --- |
| ESP-WROOM-32 | Use the ESP32 OTA download <https://micropython.org/download/esp32-ota/> |
| ESP32 | Use the ESP32 download <https://micropython.org/download/esp32/> |

## Development Setup

- Install the linter-pylint package in Atom IDE
- Install pymakr package in Atom IDE
- while in settings, go to the Pymakr package under installed packages and configure Pymakr to use your device addresses. On Windows you can type in COM3 or COM4. On Mac/*nux, you can add the path to the tty device.

## Loading Data on the ESP32s

### Atom IDE Usage

Download or clone the lamp-os folder to your PC from github

In Atom, File > Open folder... > choose lamp-os/src

Plug your ESP32 board into a USB port

Navigate to Packages > Pymakr > PyCom Console

In the top left tab of your Pymakr console, choose your project folder eg. src

In the connection tab, choose your COM or tty port and click connect

The left side bar will go green once connected. Use the upload button on the left side to upload your code to the ESP32

## Lamp creation

To create a lamp, create `src/lamps/my_lampname.py`.

Reference your lamp by making a copy of `src/main_sample.py` to `src/main.py` and update the reference to your lamp name `import lamps.my_lampname`

Have a look at other examples of lamps to get an idea of the capabilities of LampOS

## Application Structure

- `src/` -  is where all the code for a lamp lives, this gets flashed to devices (in entirety, so any lamp could really switch to another)
- `src/components` - The hardware definitions, drivers and synchronous initialization items are classified as components
- `src/behaviours` - Asynchronous tasks that run and give the lamp unique features, long running tasks and puzzles
- `src/lamp_core` - Core code that can be leveraged for all lamps.
- `src/utils` - Utility files not useful for all lamps, but handy to have around
- `src/vendor` - Any borrowed python code from the community goes here. Please make sure to retain a reference to github in the entry point file
- `src/lamps` - Each lamp will have its own entry that can be checked into the project. Your lamp is loaded from `main.py`

Note every new folder should include a `__init__.py` file to keep module discovery working properly

## Animation

LampOS works with small building blocks called `Behaviours`. In order to send information to the LED arrays, you'll need to make some building blocks of your own.

`AnimatedBehaviours` give you a toolkit for making unique lamp lighting effects. The most basic animated behaviour boilerplate looks like this:

```python
class MyAnimation(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        #initialize your animation's variables here

    async def draw(self):
        # change individual LED pixels here 

        await self.next_frame()

    async def control(self):
        while True:
            # control your animation here             

            await asyncio.sleep(0)
```

**lamp concept and buffers** - the lamp is divided into two parts, the base glass and the upper lampshade part. You can address their individual banks of colors using `self.lamp.base.buffer` or `self.lamp.shade.buffer` changes made to the buffer will write to the LED strips at around 30 frames per second.

Example: Applying a single color to the lamp's base and shade. `default_pixels` pulls up the default lamp colors as a big list. Let's experiment with a red color in the default_pixels list `[(255, 0, 0, 0), (255, 0, 0, 0)], ...]`. The following function when played will illuminate the strip in a full red color.

```python
    class LampIdle(AnimatedBehaviour)
        async def draw(self):
            self.lamp.shade.buffer = self.lamp.shade.default_pixels.copy()
            self.lamp.base.buffer = self.lamp.base.default_pixels.copy()

            await self.next_frame()
```

A slightly more complicated Example: adding a randomized point of light and fading it in and out. The control block is responsible for picking a random location for the light between pixels 12 and 20 every time the drawing phase ends.  

```python
    class Sun(AnimatedBehaviour):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.sun_position = 12

        async def draw(self):
            self.lamp.base.buffer[self.sun_position] = pingpong_fade(self.lamp.base.buffer[self.sun_position], (255, 180, 40, 120), self.lamp.base.buffer[self.sun_position], self.frames, self.frame)

            await self.next_frame()

        async def control(self):
            while True:
                if self.frame == 0:
                    self.sun_position = choice(range(12, 20))

                await asyncio.sleep(0)

```

### Initializing a lamp

Behaviours must be setup before the lamp is initialized with `wake()`. An Example that would setup a gentle fade in to a solid red color with a breathing point light between LED 12 and 20 would look like this.

```python
config = {
    "lamp": { "default_behaviours": False, "debug": True }
}

# Begin a clean lamp with no built ins and a deep red color
my_lamp = StandardLamp('mylamp', "#FF0000", "#FF0000", config)

# add your AnimatedBehaviours 
my_lamp.add_behaviour(LampFadeIn(my_lamp, frames=30, chained_behaviors=[LampIdle]))
my_lamp.add_behaviour(LampIdle(my_lamp, frames=1))
my_lamp.add_behaviour(Sun(my_lamp, frames=200, auto_play=True))

# instruct the lamp to turn on
my_lamp.wake()
```

### AnimatedBehaviour API

**async draw()** - modify the buffer for the LED pixels in this function. AnimatedBehaviours begin in a stopped state, so they my receive a `play` command to begin modifying the LED strips. This can be done using chained_behaviors, or by having your controller call self.play(). It is required

**async control()** - play, stop, pause or change the patterns of behaviour in this function. This method is called 5 seconds after lamp startup and will loop forever using the pattern above. It is optional

**async next_frame()** - signal the draw phase to proceed to the next behaviour in the chain. Following the last behaviour, the lamp will flush the completed image to the LED strip. Behaviours can stack on top of each other and steal one another's pixels from the buffer

**frames** - the total number of frames to render. The lamp runs at 30FPS so you can divide accordingly to get a rough idea of the speed of your effect

**frame** - the current frame of animation being drawn

**current_loop** - get a count of the number of times the behaviour has drawn

**animation_state** - get the state of playback for the draw phase. it can be one of: `PLAYING, STOPPING, STOPPED, PAUSING, PAUSED`

**chained_behaviours** - In the cases where a Behaviour plays another one, this variable collects all AnimatedBehaviours to `play()` as a list

**immediate_control** - If set to True, the control phase will startup with the lamp instead of waiting 5 seconds. defaults to False

**use_in_home_mode** - if set to False, the behaviour will not be loaded to save lamp owners from distracting animations while the lamp is at home

**auto_play** - if set to True, the animation will start 5 seconds after boot. use this instead of chained behaviors where a seamless transition is not needed

**play()** - play the animation

**stop()** - stop the animation gracefully by drawing all the way to the end of the frames

**pause()** - immediately stop the animation at its current frame

**reset()** - restart the draw phase to the first frame

**is_last_frame()** - True if the animation is complete

## Components and Vendor libs

Sometimes drivers and 3rd party libraries are a good choice for simple implementation of sensor handling, LED strip compatibility, Color and graphics utilities etc.

The `vendor` folder is used for unmodified micropython source. Anything modified or proxied for lamp use should go in the components folder

The `components` folder contains glue logic or implementations for addressing all of the attached IO. Generally, it's good practice to add to the components so others can integrate with similar sensors and LED strips

### DMX Details

Lamps can receive DMX signals for those looking to synchronize them with a stage. The LampDmx behavior will listen for 10 channels of DMX given a start address between 3-502. The start channel is configurable in the web portal on the lamp, but the default is channel 3

You will also need to define a fixture that broadcasts [176, 11] on channel 1 and 2 to activate the lamps. They derive a clock from channel 1 and 2. If the clock is absent, they will revert to their own personality after a few seconds

The DMX channel/address scheme is:

- 1-4  RGBW Shade
- 5-8  RGBW Base
- 9-10 Accessory 1 and 2

You will need to purchase a MAX3485 dev board (the 3.3V compatible version of the popular RS-485 serial driver). The board size should be: 19.3mmx13.3mm. The pinout is:

```
3485 TX  -> ESP Pin D19 and ESP PIN D21
3485 EN  -> ESP Pin D5
3485 VCC -> ESP 3V3
3485 GND -> ESP GND
3485 B   -> Black Wire  -> XLR & MiniXLR Pin 2 / Tip
3485 A   -> Yellow Wire -> XLR & MiniXLR Pin 3 / Ring
3485 Gnd -> Red Wire    -> XLR & MiniXLR Pin 1 / Sleeve
```

## Social Features  

The lamps are aware of how many other lamps are nearby, as well as their names and configured base and shade colors. Some examples of things that can be done with this:

- A lamp could have a list of lamps it considers friends and ones it doesn't like as much. It could change it's color based on the presence of or ratio of friends to other lamps.
- A lamp could simply be smitten with another lamp, and it could blush by a section of it's shade shifting to a reddish color when the other lamp is near.
- A lamp could be shy, and get much dimmer when too many other lamps are around.
- A lamp could be afraid of a specific lamp and flicker erratically when that lamp initially shows up, and then going more dim while it's around.
- A lamp could be a trickster and simply at random switch colors for very brief (half seconds) periods. Maybe occasionally drift its colors over the course of the night.
- A lamp could collect base colors of nearby lamps and comprise its base color as a stacked rainbow of those colors instead of a solid color
