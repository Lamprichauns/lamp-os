import uasyncio as asyncio
from utils.gradient import create_gradient
from utils.fade import fade
from lamp_core.behaviour import AnimatedBehaviour, Behaviour, AnimationStyle
from lamp_core.custom_lamp import CustomLamp
from components.led.led_strip_2812_rgb import LedStrip2812RGB
from vendor.easing import LinearInOut
from behaviours.defaults import LampFadeIn

# for ease of use, you can define a config to flow into all the components
config = {
    "lamp": { "name": "custom" },
    "shade": { "pin": 13, "pixels": 40, "default_color": "#5b4711" },
    "base": { "pin": 12, "pixels": 5, "default_color": "#270000" },
}

animated_lamp = CustomLamp(config["lamp"]["name"])
animated_lamp.shade = LedStrip2812RGB(config["shade"]["default_color"], config["shade"]["pin"], config["shade"]["pixels"])
animated_lamp.base = LedStrip2812RGB(config["base"]["default_color"], config["base"]["pin"], config["base"]["pixels"])

class WarpDrive(AnimatedBehaviour):
    async def run(self):
        warp_drive_pattern = create_gradient((35, 7, 0, 0), (90, 23, 0, 0), 40)

        while True:
            warp_drive_pattern_start = warp_drive_pattern
            warp_drive_pattern = warp_drive_pattern[4:] + warp_drive_pattern[:4]
            colors = {}
            for i in range(40):
                colors[i] = fade(warp_drive_pattern_start[i], warp_drive_pattern[i], self.frame, self.frames, LinearInOut)

            self.lamp.shade.draw(colors)
            await self.next_frame()

class WarningLights(AnimatedBehaviour):
    async def run(self):
        light = (255, 255, 255, 0)

        while True:
            colors = self.lamp.shade.get_frame_buffer()

            if self.direction is True:
                colors[18] = fade(colors[18], light, self.frame, self.frames)
                colors[10] = fade(colors[10], light, self.frame, self.frames)
            else:
                colors[18] = fade(light, colors[18], self.frame, self.frames)
                colors[10] = fade(light, colors[10], self.frame, self.frames)

            self.lamp.shade.draw(colors)
            await self.next_frame()

class Draw(Behaviour):
    async def run(self):
        while True:
            self.lamp.shade.flush()
            await asyncio.sleep(0)

animated_lamp.add_behaviour(LampFadeIn(animated_lamp))
animated_lamp.add_behaviour(WarpDrive(animated_lamp, frames=30))
animated_lamp.add_behaviour(WarningLights(animated_lamp, frames=20, animate=AnimationStyle.PING_PONG))
animated_lamp.add_behaviour(Draw(animated_lamp))
animated_lamp.wake()
