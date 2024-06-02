# Adds wired DMX functionality over serial UART
# the lamp will alow addressing a single color to all leds in the base and shade
# when a DMX signal is received on the configured channels
# Each lamp will use 10 dmx channels: shade(r,g,b,w),base(r,g,b,w), acc1, acc2
#
# Also configure a 2 channel fixture at channel 1 with constant value of [176, 11]
# as the lamps will use this to sync. If this signal ends, the lamps will return
# to themselves.
import uasyncio as asyncio
from machine import Pin, UART
import utime

RX_BUFFER_SIZE = 1024
DMX_DEFAULT_CHANNEL = 3
DMX_MESSAGE_SIZE = 512
DMX_SYNC_POOL = 400
DMX_SYNC_SIGNAL = b"\xB0\x0B"
DMX_BAUD_RATE = 250000
DMX_FRAME_TIMEOUT = 550000
LAMP_CHANNEL_COUNT = 10
EN_PIN = 5

class Dmx:
    def __init__(self, channel = DMX_DEFAULT_CHANNEL):
        self.last_dmx_message = None
        self.last_break_time = 0
        self.channel = channel

        # enable the RS485 interface - active low
        enable = Pin(EN_PIN)
        enable.init(enable.OUT)
        enable.value(0)

        # create an RS232 listener for DMX signals
        self.uart = UART(1, DMX_BAUD_RATE, tx=18, rx=19)
        self.uart.init(DMX_BAUD_RATE, bits=8, parity=None, stop=2, rxbuf=RX_BUFFER_SIZE)

        # add to greater event loops on enable
        self.loop = asyncio.get_event_loop()

    async def _control_loop_coro(self):
        while True:
            # Poll constantly for a sync signal on channel 1 and 2 and begin
            # parsing a DMX pool if found.
            # These functions do yield to RTOS, so they don't block the
            # lamp's other functions
            sync_offset = 0
            bytes_to_read = 0
            res = None
            message = b""

            while True:
                res = self.uart.read(DMX_SYNC_POOL)
                if res is not None:
                    sync_offset = bytes(res).find(DMX_SYNC_SIGNAL)

                    if sync_offset >= 0:
                        self.last_break_time = utime.ticks_us()
                        bytes_to_read = DMX_MESSAGE_SIZE - DMX_SYNC_POOL - sync_offset
                        break

                await asyncio.sleep(0)

            # fill the buffer to the appropriate DMX channel data size
            # to prevent UART overflows, copy all of the dmx data into the
            # message buffer
            if sync_offset >= 0  and bytes_to_read >= 0:
                message += res[sync_offset:DMX_SYNC_POOL]

                while utime.ticks_diff(utime.ticks_us(), self.last_break_time) < DMX_FRAME_TIMEOUT:
                    tmp = self.uart.read(bytes_to_read)

                    if tmp is not None:
                        message += tmp

                    message_length = len(message)

                    if message_length == DMX_MESSAGE_SIZE:
                        break

                    bytes_to_read = DMX_MESSAGE_SIZE - message_length

                # convert message to tuple of ints from a given channel
                self.last_dmx_message = tuple(message[self.channel-1:self.channel + LAMP_CHANNEL_COUNT])
                print(self.last_dmx_message)

            await asyncio.sleep(0)

    def enable(self):
        self.loop.create_task(self._control_loop_coro())
