#include <Arduino.h>
#include "../components/network/bluetooth.hpp"
#include "../util/lamp_color.hpp"

void setup() {
    Serial.begin(115200);
    LampBluetoothComponent bt("configurable", LampColor(95690659), LampColor(0));
}

void loop() {
    //get_found_lamps();
}