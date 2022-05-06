from lamp_core.behaviour import StartupBehaviour, BackgroundBehaviour
from lamp_core.custom_lamp import CustomLamp
from components.led.led_strip_2812_rgb import LedStrip2812RGB
from vendor.easing import LinearInOut

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

class LampFadeIn(StartupBehaviour):
    async def run(self):
        async with self.lamp.lock:
            self.lamp.base.off()
            self.lamp.shade.off()

            await self.lamp.base.fade(self.lamp.base.default_pixels, 40)
            await self.lamp.shade.fade(self.lamp.shade.default_pixels, 40)

class WarpDrive(BackgroundBehaviour):
    async def run(self):
        warp_drive_pattern = create_gradient((7, 30, 115, 0), (219, 233, 251, 0), 40)

        await self.lamp.shade.fade(list(warp_drive_pattern), 2)
        while True:
            async with self.lamp.lock:
                warp_drive_pattern = warp_drive_pattern[1:] + warp_drive_pattern[:1]
                await self.lamp.shade.fade(warp_drive_pattern, 2)

animated_lamp.add_behaviour(LampFadeIn)
animated_lamp.add_behaviour(WarpDrive)

animated_lamp.wake()
