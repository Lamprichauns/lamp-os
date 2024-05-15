# Adds wired DMX functionality over serial UART
# the lamp will alow addressing a single color to all leds in the base and shade
# when a DMX signal is received on the configured channels
# Each lamp will use 10 dmx channels: shade(r,g,b,w),base(r,g,b,w), acc1, acc2
# from a start address set in the initializer
# Since this is hacky, I'd refrain from using full channel values #FF
# keep lamps in their own dmx universe
import uasyncio as asyncio
from machine import Pin, UART
import utime
from lamp_core.behaviour import AnimatedBehaviour

MIN_BREAK_TIME_US = 88
MAX_BREAK_TIME_US = 6000
RX_BUFFER_SIZE = 1024
DMX_MESSAGE_SIZE = 513
DMX_SYNC_POOL = 400
DMX_BREAK_SIGNAL = b"\xff\x00\x00"
DMX_BAUD_RATE = 250000
LAMP_CHANNEL_COUNT = 10
EN_PIN = 5
ISR_PIN = 21

class LampDmx(AnimatedBehaviour):
    def __init__(self, *args, config, **kwargs):
        super().__init__(*args, **kwargs)
        self.channel = config["dmx"]["channel"] or 1
        self.break_present = False
        self.last_break_time = 0
        self.mab_present = False
        self.irq_enable = True

        # enable the RS485 interface - active low
        enable = Pin(EN_PIN)
        enable.init(enable.OUT)
        enable.value(0)

        # Interrupt to catch a break/mab signal
        bm_isr = Pin(ISR_PIN)
        bm_isr.init(bm_isr.IN)
        bm_isr.irq(handler=self.break_mab_interrupt, trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING)

        # create an RS232 listener for DMX signals
        self.uart = UART(1, DMX_BAUD_RATE, tx=18, rx=19)
        self.uart.init(DMX_BAUD_RATE, bits=8, parity=None, stop=2, rxbuf=RX_BUFFER_SIZE)

    # spy on a break in DMX messages and only when plugged into the stage
    def break_mab_interrupt(self, _):
        if self.irq_enable is False:
            return

        if not self.break_present:
            self.break_present = True
            self.last_break_time = utime.ticks_us()
            return

        if not self.mab_present and self.break_present and MIN_BREAK_TIME_US <= utime.ticks_diff(utime.ticks_us(), self.last_break_time) <= MAX_BREAK_TIME_US:
            self.mab_present = True
            self.irq_enable = False
            return

    async def draw(self):
        # fade back to default lamp once connection is lost
        await self.next_frame()

    async def control(self):
        while True:
            if self.mab_present:
                message = b""
                sync_offset = -1
                bytes_to_read = 0

                # find the first sync signal after break - asyncio can be pretty
                # slow to react, so loop through the buffer to catch up
                # These functions do yield to RTOS, so they don't block the
                # lamp's other functions
                while utime.ticks_diff(utime.ticks_us(), self.last_break_time) < 100000:
                    res = self.uart.read(DMX_SYNC_POOL)
                    if res is not None:
                        sync_offset = bytes(res).find(DMX_BREAK_SIGNAL)

                    if sync_offset >= 0:
                        bytes_to_read = DMX_MESSAGE_SIZE - DMX_SYNC_POOL - sync_offset
                        break

                # fill the buffer to the appropriate DMX channel data size
                # to prevent UART overflows, copy all of the dmx data into the
                # message buffer
                if sync_offset >= 0  and bytes_to_read >= 0:
                    message += res[sync_offset:DMX_SYNC_POOL]

                    while utime.ticks_diff(utime.ticks_us(), self.last_break_time) < 100000:
                        tmp = self.uart.read(bytes_to_read)

                        if tmp is not None:
                            message += tmp

                        message_length = len(message)

                        if message_length == DMX_MESSAGE_SIZE:
                            break

                        bytes_to_read = DMX_MESSAGE_SIZE - message_length

                    # convert message to tuple of ints from a given channel
                    dmx_message =  tuple(message[self.channel:self.channel + LAMP_CHANNEL_COUNT])

                    # copy the values to the lamp's buffers for immediate draw on next frame
                    self.lamp.shade.buffer = [dmx_message[:4]] * self.lamp.shade.num_pixels
                    self.lamp.base.buffer = [dmx_message[4:8]] * self.lamp.base.num_pixels
                self.break_present = False
                self.mab_present = False
                self.irq_enable = True

            await asyncio.sleep(0)
