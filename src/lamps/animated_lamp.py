from lamp_core.behaviour import StartupBehaviour, BackgroundBehaviour
from lamp_core.custom_lamp import CustomLamp
from components.led.led_strip_2812_rgb import LedStrip2812RGB
from vendor.easing import LinearInOut
import uasyncio as asyncio

# Create a gradient between one color and another. Input colors should be 4-tuples of RGBW
def create_gradient(color_start, color_end, steps, easing_function=LinearInOut):
    color_list = [(0, 0, 0, 0)] * steps
    for i in range(steps):
        color_list[i] = (
            int(easing_function(start = color_start[0], end = color_end[0], duration = steps)(i)),
            int(easing_function(start = color_start[1], end = color_end[1], duration = steps)(i)),
            int(easing_function(start = color_start[2], end = color_end[2], duration = steps)(i)),
            int(easing_function(start = color_start[3], end = color_end[3], duration = steps)(i)),
        )

    return color_list

# for ease of use, you can define a config to flow into all the components
config = {
    "lamp": { "name": "custom" },
    "shade": { "pin": 13, "pixels": 40, "default_color": "#5b4711" },
    "base": { "pin": 12, "pixels": 5, "default_color": "#270000" },
}

animated_lamp = CustomLamp(config["lamp"]["name"])
animated_lamp.shade = LedStrip2812RGB(config["shade"]["default_color"], config["shade"]["pin"], config["shade"]["pixels"])
animated_lamp.base = LedStrip2812RGB(config["base"]["default_color"], config["base"]["pin"], config["base"]["pixels"])

class WarpDrive(BackgroundBehaviour):
    def __init__(self, lamp):
        super().__init__(lamp)
        self.step = 0

    async def run(self):
        warp_drive_pattern = create_gradient((5, 22, 31, 0), (23, 50, 80, 0), 40)

        while True:
            warp_drive_pattern_start = warp_drive_pattern
            warp_drive_pattern = warp_drive_pattern[3:] + warp_drive_pattern[:3]
            colors = {}
            for i in range(40):
                colors[i] = (
                    LinearInOut(start = warp_drive_pattern_start[i][0], end = warp_drive_pattern[i][0], duration = 90)(self.step),
                    LinearInOut(start = warp_drive_pattern_start[i][1], end = warp_drive_pattern[i][1], duration = 90)(self.step),
                    LinearInOut(start = warp_drive_pattern_start[i][2], end = warp_drive_pattern[i][2], duration = 90)(self.step),
                    LinearInOut(start = warp_drive_pattern_start[i][3], end = warp_drive_pattern[i][3], duration = 90)(self.step)
                )

            self.lamp.shade.draw(colors)
            self.step += 1
            if self.step == 90:
                self.step = 0

            await asyncio.sleep_ms(1)

class WarningLights(BackgroundBehaviour):
    def __init__(self, lamp):
        super().__init__(lamp)
        self.step = 0

    async def run(self):
        direction = True
        while True:
            light = (255, 255, 255, 0)

            colors = self.lamp.shade.frame_buffer

            if direction is True:
                colors[18] = (
                    LinearInOut(colors[18][0], light[0], duration = 90)(self.step),
                    LinearInOut(colors[18][1], light[1], duration = 90)(self.step),
                    LinearInOut(colors[18][2], light[2], duration = 90)(self.step),
                    LinearInOut(colors[18][3], light[3], duration = 90)(self.step),
                )
                colors[10] = (
                    LinearInOut(colors[10][0], light[0], duration = 90)(self.step),
                    LinearInOut(colors[10][1], light[1], duration = 90)(self.step),
                    LinearInOut(colors[10][2], light[2], duration = 90)(self.step),
                    LinearInOut(colors[10][3], light[3], duration = 90)(self.step),
                )

            if direction is False:
                colors[18] = (
                    LinearInOut(light[0], colors[18][0], duration = 90)(self.step),
                    LinearInOut(light[1], colors[18][1], duration = 90)(self.step),
                    LinearInOut(light[2], colors[18][2], duration = 90)(self.step),
                    LinearInOut(light[3], colors[18][3], duration = 90)(self.step),
                )
                colors[10] = (
                    LinearInOut(light[0], colors[10][0], duration = 90)(self.step),
                    LinearInOut(light[1], colors[10][1], duration = 90)(self.step),
                    LinearInOut(light[2], colors[10][2], duration = 90)(self.step),
                    LinearInOut(light[3], colors[10][3], duration = 90)(self.step),
                )

            self.lamp.shade.draw(colors)
            self.step += 1
            if self.step == 90:
                self.step = 0
                direction = not direction
            await asyncio.sleep_ms(1)

class Draw(BackgroundBehaviour):
    async def run(self):
        while True:
            self.lamp.shade.flush()
            await asyncio.sleep_ms(1)

animated_lamp.add_behaviour(WarpDrive)
animated_lamp.add_behaviour(WarningLights)
animated_lamp.add_behaviour(Draw)
animated_lamp.wake()
