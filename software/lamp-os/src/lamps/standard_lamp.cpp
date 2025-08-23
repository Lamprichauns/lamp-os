#include <Arduino.h>
#include "../components/network/bluetooth.hpp"
#include "../util/color.hpp"

lamp::BluetoothComponent bt;

void setup() {
    Serial.begin(115200);
    bt.begin("configurable", lamp::Color(95690659), lamp::Color(0));
}

void loop() {
    if(millis()%500 == 0) {
        bt.getLamps();
    }
}