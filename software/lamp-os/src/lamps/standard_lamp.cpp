#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "SPIFFS.h"

#define LED_PIN 12
#define LED_COUNT 9

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  bt.begin("configurable", lamp::Color(95690659), lamp::Color(0));
  wifi.begin("configurable");
  strip.begin();
  strip.show();
}

void loop() {
#ifdef LAMP_DEBUG
  long microtime = micros();
#endif
  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    strip.fill((uint32_t)((artnetData[0].w << 24) | (artnetData[0].r << 16) |
                          (artnetData[0].g << 8) | (artnetData[0].b)));
    strip.show();
  }

  wifi.tick();
#ifdef LAMP_DEBUG
  if (millis() % 500 == 0) {
    Serial.printf("Main loop took: %duS\n", micros() - microtime);
  }
#endif
}