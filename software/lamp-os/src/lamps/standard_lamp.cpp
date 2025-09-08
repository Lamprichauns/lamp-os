#include "./standard_lamp.hpp"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Preferences.h>

#include <string>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "./behaviors/dmx.hpp"
#include "./behaviors/social.hpp"
#include "./config/config.hpp"
#include "./core/animated_behavior.hpp"
#include "./core/compositor.hpp"
#include "./core/frame_buffer.hpp"
#include "./globals.hpp"
#include "SPIFFS.h"

Adafruit_NeoPixel shadeStrip(LAMP_DEFAULT_NUMBER_PIXELS, LAMP_SHADE_PIN,
                             NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel baseStrip(LAMP_DEFAULT_NUMBER_PIXELS, LAMP_BASE_PIN,
                            NEO_GRBW + NEO_KHZ800);
Preferences prefs;
lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;
lamp::Compositor compositor;
lamp::FrameBuffer shade;
lamp::FrameBuffer base;
lamp::DmxBehavior shadeDmxBehavior;
lamp::DmxBehavior baseDmxBehavior;
lamp::SocialBehavior shadeSocialBehavior;

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  lamp::Config config = lamp::Config(&prefs);
  bt.begin(config.lamp.name, config.base.colors[0], config.shade.colors[0]);
  wifi.begin(&config);
  shade.begin(config.shade.colors[0], config.shade.px, &shadeStrip);
  base.begin(config.base.colors.at(config.base.ac), config.base.px, &baseStrip);
  shadeDmxBehavior = lamp::DmxBehavior(&shade, 0);
  baseDmxBehavior = lamp::DmxBehavior(&base, 0);
  shadeSocialBehavior = lamp::SocialBehavior(&shade, 1200);
  compositor.begin({&shadeSocialBehavior}, {&shade, &base});
};

void loop() {
  shadeSocialBehavior.updateFoundLamps(bt.getLamps());
  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    shadeDmxBehavior.setColor(artnetData[0]);
    shadeDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
    baseDmxBehavior.setColor(artnetData[1]);
    baseDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
  }
  wifi.tick();
  compositor.tick();
};