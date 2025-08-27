#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "./behaviors/dmx.cpp"
#include "./behaviors/idle.cpp"
#include "./core/animated_behavior.hpp"
#include "./core/compositor.hpp"
#include "./core/frame_buffer.hpp"
#include "SPIFFS.h"
#include "globals.hpp"

Adafruit_NeoPixel shadeStrip(LAMP_DEFAULT_NUMBER_PIXELS, LAMP_SHADE_PIN,
                             NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel baseStrip(LAMP_DEFAULT_NUMBER_PIXELS, LAMP_BASE_PIN,
                            NEO_GRBW + NEO_KHZ800);

lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;
lamp::Compositor compositor;
lamp::FrameBuffer shade;
lamp::FrameBuffer base;
lamp::DmxBehavior shadeDmxBehavior;
lamp::DmxBehavior baseDmxBehavior;

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  bt.begin("configurable", lamp::Color(95690659), lamp::Color(0));
  wifi.begin("configurable");
  shade.begin(lamp::Color(95690659), LAMP_DEFAULT_NUMBER_PIXELS, &shadeStrip);
  base.begin(lamp::Color(0x30, 0x07, 0x83, 0x00), LAMP_DEFAULT_NUMBER_PIXELS,
             &baseStrip);
  shadeDmxBehavior = lamp::DmxBehavior(&shade, 0);
  baseDmxBehavior = lamp::DmxBehavior(&base, 0);
  compositor.begin({new lamp::IdleBehavior(&shade, 0, false, true),
                    new lamp::IdleBehavior(&base, 0, false, true),
                    &shadeDmxBehavior, &baseDmxBehavior},
                   {&shade, &base});
};

void loop() {
  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    shadeDmxBehavior.setColor(artnetData.at(0));
    shadeDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
    baseDmxBehavior.setColor(artnetData.at(1));
    baseDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
  }
  wifi.tick();
  compositor.tick();
};