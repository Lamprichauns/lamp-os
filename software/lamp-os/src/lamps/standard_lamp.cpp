#include <Arduino.h>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "SPIFFS.h"

lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;

void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);
  bt.begin("configurable", lamp::Color(95690659), lamp::Color(0));
  wifi.begin("configurable");
}

void loop() {
  if (millis() % 500 == 0) {
    bt.getLamps();
  }

  wifi.tick();
}