# Lamp Build Guide

As part of the process for making a lamp using Lamp OS, you'll need to convert the lamp to use a DC power source and add LEDs to the shade and base. This guide shows how we usually convert a lamp as an illustrated guide

## Prerequisites

To begin your build, you'll need a number of parts and tools ready to go

### Lamp Parts

More technical details of the parts you'll need to purchase can be found on the main readme

- A working lamp and shade
- USB Type A plug
- Some 4in cable ties
- An ESP32 board programmed with Lamp OS
- about 1.5m of 5V Neopixel style LED strip
- Some 24ga hookup wire in blue, red, black and yellow
- 3D printed bulb (optional)
- a pack of 24ga marettes (optional)
- a pack of lamp hardware (extra nuts/bolts/washers)
- a multimeter

### Tools

- hand drill and some assorted metal drill sizes
- wire cutters and strippers
- soldering iron
- some locking pliers/Visegrips for removing lamp hardware
- screwdrivers for lamp hardware (small phillips and slot head)
- 3d printer (optional)

## Parts of a Lamp

![Basic lamp terminology](images/lamp-wiring-socket.png)

## Disassembly

To begin, you'll have to disassemble your lamp. The easiest way I've found to do this:

 1. remove the lampshade and light bulb
 2. unscrew the locking screw just below the bulb socket if it's there
 3. grab the base of the lamp and the socket and gently rotate them to loosen the entire internal assembly.
 4. The socket should completely unscrew from the rest of lamp hardware with enough turns
 5. clip the lamp cord at the socket side (shown in the figure above). Once the lamp cord is cut, you can pull it out of the lamp. It'll be reused in a later step
 6. take all the rest of the hardware apart and take some photos of the order that the parts go together
 7. put all the glass aside in a safe spot until final assembly

## Things to Build Ahead of Time

### Build a 3d printed shade bulb

Cut 35 LEDs from the strip and solder 3 wires - you may need to pull the LED strip out a little:

- black -> gnd
- yellow -> Din
- red -> +5V

![Soldering the Shade LEDs](images/shade-led-1.jpg)

Push the wires through the lower hole and clean up the 3d print so it's square as possible

![Push wires into the bulb](images/shade-led-2.jpg)

Wrap the LEDs around the bulb and tuck them into the top hole

![The bulb should hold the LEDs in place](images/shade-led-3.jpg)

Once the LEDs are in place, wrap them in clear tape to fasten them for good

![Wrap the LEDs with a couple layers of tape](images/shade-led-4.jpg)

### Build the PCB

Cut 4 wires around 6" each

- black -> GND
- red -> +5v/VIN
- yellow -> D12
- blue -> D14

You can zip tie these wires from any of the board's screw holes next to the D32 or EN pins.

![PCB wiring](images/pcb-1.jpg)
![PCB Wiring](images/pcb-2.jpg)

## Assembly

Now the lamp is in pieces, we can begin converting it. The goal of the lamp conversion is to wire it up like this

![Assembly plan](images/Lamp-wiring-dc.png)

### Build Steps

Begin by drilling a few holes in the base. These holes should be as close to the lower locknut as possible.

![Drilling the lamp base](images/photo_2025-04-13_17-59-24.jpg)

Place a zip tie through the two smaller holes

![Cord zip tie](images/photo_2025-04-13_17-59-26.jpg)

Install the lamp pipe into the lamp base and install a top and bottom nut and bottom lock washer to secure the pipe in place. You may need to test fit the base glass a few times to ensure the pipe is in the right position

![Install the lamp pipe](images/photo_2025-04-13_17-59-37.jpg)

Cut and remove the lamp cord from the socket

![Cut the lamp cord to prepare to convert it to USB](images/photo_2025-04-13_17-59-28.jpg)

Strip the cable and solder it to the USB connector. The ribbed or striped side of the cable should be ground

![Solder the USB connector](images/photo_2025-04-13_17-59-30.jpg)

Double check your work on soldering the connector with a multimeter. Plug the connector into the battery. Connect the black lead to the ribbed/striped side of the lamp cord and the red lead to the other. In DC voltage mode, make sure 2-5V is coming out of the connector and in the right polarity.

![Check for 2-5V on the other end of the lamp cord](images/photo_2025-04-13_17-59-32.jpg)

On the base LED strip, solder 3 wires:

- black -> gnd
- blue -> Din
- red -> +5V

And wrap it around the lamp pipe to the top of the base glass. It's generally about 30-40 LEDs

![Wrap the LEDs around the pipe](images/photo_2025-04-13_17-59-39.jpg)

Tie off the base LEDs with a couple of wire ties

![More wire ties](images/photo_2025-04-13_17-59-41.jpg)

We can now begin putting the lamp glass and socket back together. Cut another 3 red, black and blue wires to feed down from the bottom of the socket down to the bottom of the base

![Data and power routed back to the base](images/photo_2025-04-13_17-59-43.jpg)

Put the top of the socket back into place and place the lamp PCB into the socket

![Socket PCB wiring](images/photo_2025-04-13_17-59-45.jpg)

Use marettes to connect all the like colored wires together

![Strip and connect all the wires](images/photo_2025-04-13_17-59-50.jpg)

Once all the connections are made, clip the bulb onto the socket by pushing all the wires into the 3d printed bulb and then pushing down until it clicks into place. If you don't have a 3d printer, you can wrap the LEDs around the harp instead

![Clip on the 3d printed bulb](images/photo_2025-04-13_17-59-56.jpg)

Now to wire the base. Connect the striped or ridged lamp cord to the black wire. The red to the other lamp cord. Also add the red wire and black wires from the base LEDs and use a marette or solder/heatshrink to connect them all. the blue and blue wire will connect in the same way. Plug in the lamp and make sure it turns on. If it works, then strap them all to the base using the wire tie placed earlier

![Base wiring](images/photo_2025-04-13_17-59-47.jpg)

> Note: this build includes DMX wiring, the large black cable, which is based on an optional prototype

Add the lampshade and plug the lamp in! With any luck, it'll spin up with the built in colors. From here, you can update your lamp by finding `lamp-configurable` in your wifi list and visiting <http://192.168.4.1> from any web browser to configure the lamp's name and colors.

![Finished lamp](images/IMG_9844.jpg)

This ends the tutorial for the basic build for the lamps
