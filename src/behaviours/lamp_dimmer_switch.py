# Handle a dimmer effect with a physical pushbutton
import uasyncio as asyncio
from machine import Pin
from lamp_core.behaviour import AnimatedBehaviour
from utils.color_tools import darken
from utils.fade import fade

class LampDimmer(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.is_dimmed = False
        self.transition = False

    async def draw(self):
        if self.transition:
            if self.is_dimmed:
                for i in range(self.lamp.base.num_pixels):
                    self.lamp.base.buffer[i] = fade(self.lamp.base.buffer[i], darken(self.lamp.base.buffer[i], self.amount), self.frames-1, self.frame)

                for i in range(self.lamp.shade.num_pixels):
                    self.lamp.shade.buffer[i] = fade(self.lamp.shade.buffer[i], darken(self.lamp.shade.buffer[i], self.amount), self.frames-1, self.frame)
            else:
                for i in range(self.lamp.base.num_pixels):
                    self.lamp.base.buffer[i] = fade(darken(self.lamp.base.buffer[i], self.amount), self.lamp.base.buffer[i], self.frames-1, self.frame)

                for i in range(self.lamp.shade.num_pixels):
                    self.lamp.shade.buffer[i] = fade(darken(self.lamp.shade.buffer[i], self.amount), self.lamp.shade.buffer[i], self.frames-1, self.frame)

        if not self.transition and self.is_dimmed:
            for i in range(self.lamp.base.num_pixels):
                self.lamp.base.buffer[i] = darken(self.lamp.base.buffer[i], self.amount)

            for i in range(self.lamp.shade.num_pixels):
                self.lamp.shade.buffer[i] = darken(self.lamp.shade.buffer[i], self.amount)

        await self.next_frame()

    async def control(self):
        push_button = Pin(4, Pin.IN, Pin.PULL_DOWN)
        self.play()

        while True:
            if self.frame == 0:
                button_state = push_button.value()

                if button_state == 0:
                    if not self.is_dimmed:
                        self.transition = True

                    self.is_dimmed = True
                else:
                    if self.is_dimmed:
                        self.transition = True

                    self.is_dimmed = False

            await asyncio.sleep(0)
