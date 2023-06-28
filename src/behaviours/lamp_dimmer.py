# Handle a dimmer effect when a magnet is held close to the ESP32 hall effect sensor
import esp32
import uasyncio as asyncio
from lamp_core.behaviour import AnimatedBehaviour
from utils.color_tools import darken
from utils.fade import fade

class LampDimmer(AnimatedBehaviour):
    def __init__(self, *args, threshold=15, baseline=205, amount=50, **kwargs):
        super().__init__(*args, **kwargs)
        self.rolling_sum = 0
        self.is_dimmed = False
        self.transition = False
        self.low = baseline - threshold
        self.high = baseline + threshold
        self.amount = amount

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
        self.play()

        while True:
            self.rolling_sum += int(esp32.hall_sensor())

            if self.frame == 0:
                self.transition = False

                sampled_value = self.rolling_sum//self.frames
                if (self.low >= sampled_value or sampled_value >= self.high):
                    if not self.is_dimmed:
                        self.transition = True

                    self.is_dimmed = True
                else:
                    if self.is_dimmed:
                        self.transition = True

                    self.is_dimmed = False

                self.rolling_sum = 0

            await asyncio.sleep(0)
