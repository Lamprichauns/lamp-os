#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "./core/frame_buffer.hpp"
#include "SPIFFS.h"

#define LED_PIN 12
#define LED_COUNT 9

Adafruit_NeoPixel shadeStrip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;
lamp::FrameBuffer shade;

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  bt.begin("configurable", lamp::Color(95690659), lamp::Color(0));
  wifi.begin("configurable");
  shade.begin(lamp::Color(95690659), 40, &shadeStrip);
}

void loop() {
  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    shade.fill(artnetData[0]);
    shade.flush();
  }

  wifi.tick();
}