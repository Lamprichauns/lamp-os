#ifndef LAMP_BLUETOOTH_COMPONENT_H
#define LAMP_BLUETOOTH_COMPONENT_H

#include <Arduino.h>
#include "../../util/lamp_color.hpp"

// Lamp Identifier
#define BLE_MAGIC_NUMBER 42069 //unsigned short - packed as little endian from original Python

// Scan every INTERVAL for WINDOW
#define BLE_GAP_SCAN_INTERVAL_MS 20
#define BLE_GAP_SCAN_WINDOW_MS 10

// Advertise every INTERVAL
#define BLE_GAP_ADV_INTERVAL_MS 10

// Scan time
#define BLE_GAP_SCAN_TIME 500

// Power level in DB
#define BLE_POWER_LEVEL 4

class LampBluetoothComponent {
    public:
        LampBluetoothComponent(std::__cxx11::string name, LampColor base_color, LampColor shade_color);
        int get_found_lamps();
};

#endif