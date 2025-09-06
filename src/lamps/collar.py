# This custom lamp type is used for decorative tracking collars
import uasyncio as asyncio
from components.network.access_point import AccessPoint
from lamp_core.behaviour import AnimatedBehaviour, AnimationState
from lamp_core.standard_lamp import StandardLamp
from utils.color_tools import darken
from utils.fade import pingpong_fade, fade
from behaviours.configurator import Configurator, configurator_load_data
from behaviours.lamp_brightness import LampBrightness

# Define what we'll be setting in the web app
config = configurator_load_data({
    "shade": { "pixels": 9, "color":"#ffffff", "pin": 12 },
    "base": { "pixels": 8, "color":"#300783", "pin": 14 },
    "lamp": { "name": "configurable", "brightness": 100, "home_mode": False },
    "wifi": { "ssid": "lamp-configurable" },
    "dmx": { "channel": 4 }
})

config["wifi"]["ssid"] = "lamp-%s" % (config["lamp"]["name"])

# Start a standard lamp and extend it to be a Wifi Access Point
configurable = StandardLamp(name=config["lamp"]["name"], base_color=config["base"]["color"], shade_color=config["shade"]["color"], config_opts=config)
configurable.access_point = AccessPoint(ssid=config["wifi"]["ssid"], password="123456789")

class CollarBlink(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        #self.immediate_control = True

    async def draw(self):
        colors = self.lamp.shade.buffer
        for j in range(self.lamp.shade.num_pixels):
            self.lamp.shade.buffer[j] = pingpong_fade(colors[j], (0, 0, 0, 0), colors[j], self.frames, self.frame)

        await self.next_frame()

class CollarSocial(AnimatedBehaviour):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arrived = None
        self.frames = self.frames or 1200
        self.ease_frames = 60
        self.use_in_home_mode = False
        self.think_pattern = [(0,0,0,0)] * self.lamp.base.num_pixels
        self.think_pattern[0] = self.lamp.base.default_pixels[0]
        self.think_pattern[1] = self.lamp.base.default_pixels[0]
        self.think_pattern[3] = self.lamp.base.default_pixels[0]

    async def draw(self):
        for k in range(self.lamp.shade.num_pixels):
            if self.frame < self.ease_frames:
                self.lamp.shade.buffer[k] = fade(self.lamp.shade.buffer[k], self.arrived["base_color"], self.ease_frames, self.frame)

            elif self.frame > self.frames-self.ease_frames:
                self.lamp.shade.buffer[k] = fade(self.arrived["base_color"], self.lamp.shade.buffer[k], self.ease_frames, self.frame % self.ease_frames)

            else:
                self.lamp.shade.buffer[k] = self.arrived["base_color"]

        self.think_pattern = self.think_pattern[1:] + self.think_pattern[:1]

        for l in range(self.lamp.base.num_pixels):
            self.lamp.base.buffer[l] = self.think_pattern[l]

        await self.next_frame()

    async def control(self):
        while True:
            # wait for lamp to be started up for a while on first boot
            await asyncio.sleep(15)

            if self.animation_state not in (AnimationState.PLAYING, AnimationState.STOPPING):
                arrived = await self.lamp.network.arrived()
                self.arrived = arrived
                print("%s has arrived" % (arrived["name"]))

                self.play()
                self.stop()

# To handle pastel colors and cool whites, we can darken the rgb values for a normal 40x40 lamp to not draw too much current
# tested with shade: #D486FE (pastel purple), base: #FF9494 (pastel rose) to get a 5V voltage of 4.8V
# This will ensure lamps can stay electrically stable for many hours even on extreme color settings
# This routine won't modify warm white values
for i in range(config["base"]["pixels"]):
    if configurable.base.default_pixels[i][3] == 255:
        break

    configurable.base.default_pixels[i] = darken(configurable.base.default_pixels[i], 30)

for i in range(config["shade"]["pixels"]):
    if configurable.shade.default_pixels[i][3] == 255:
        break

    configurable.shade.default_pixels[i] = darken(configurable.shade.default_pixels[i], 30)

configurable.add_behaviour(CollarBlink(configurable, frames=15, auto_play=True))
configurable.add_behaviour(CollarSocial(configurable, frames=300))
configurable.add_behaviour(LampBrightness(configurable, frames=1, brightness=configurable.brightness))
configurable.add_behaviour(Configurator(configurable, config=config))
configurable.wake()
