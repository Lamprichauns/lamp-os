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

#define LED_PIN 12
#define LED_COUNT 9

Adafruit_NeoPixel shadeStrip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;
lamp::Compositor compositor;
lamp::FrameBuffer shade;
lamp::DmxBehavior dmxBehavior;

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  bt.begin("configurable", lamp::Color(95690659), lamp::Color(0));
  wifi.begin("configurable");
  shade.begin(lamp::Color(95690659), 40, &shadeStrip);
  dmxBehavior = lamp::DmxBehavior(&shade, 0);
  compositor.begin(
      {new lamp::IdleBehavior(&shade, 0, false, true), &dmxBehavior}, {&shade});
};

void loop() {
  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    dmxBehavior.setColor(artnetData.at(0));
  }
  wifi.tick();
  compositor.tick();
};