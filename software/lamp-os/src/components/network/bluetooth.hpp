#ifndef LAMP_BLUETOOTH_COMPONENT_H
#define LAMP_BLUETOOTH_COMPONENT_H

#include <Arduino.h>
#include "./lamp_pool.hpp"

// Lamp manufacturer identifier
#define BLE_MAGIC_NUMBER 42069

// Scan every INTERVAL for WINDOW
#define BLE_GAP_SCAN_INTERVAL_MS 20
#define BLE_GAP_SCAN_WINDOW_MS 10

// Advertise every INTERVAL
#define BLE_GAP_ADV_INTERVAL_MS 500

// Scan time
#define BLE_GAP_SCAN_TIME 500

// Tx power level in DB
// @see platformio build flag MYNEWT_VAL_BLE_LL_TX_PWR_DBM as they must match
#define BLE_POWER_LEVEL 4

// Minimum RSSI to be included/updated in the lamp pool
#define BLE_MINIMUM_RSSI_VALUE -94

/**
 * @brief Entrypoint class to advertise and track lamps by Bluetooth LE
 */
class LampBluetoothComponent {
public:
    LampBluetoothComponent(std::__cxx11::string name, LampColor base_color, LampColor shade_color);

    /**
     * @brief get a listing of all lamps within acceptable signal strength limits
     * @return vector of all found lamps
     */
    void get_all_lamps();
};

#endif