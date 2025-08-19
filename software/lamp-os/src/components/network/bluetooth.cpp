
/**
 *  Lamp Bluetooth Management
 *
 *  Based on: NimBLE_Async_client Demo
 *
 *  Demonstrates asynchronous client operations.
 *
 *  Created: on November 4, 2024
 *      Author: H2zero
 */
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "./bluetooth.hpp"
#include "../../util/lamp_color.hpp"

class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
        if (advertisedDevice->haveName() && advertisedDevice->getName() == "NimBLE-Server") {
            Serial.printf("Found Our Device\n");
        }
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.printf("Scan Ended\n");
        NimBLEDevice::getScan()->start(BLE_GAP_SCAN_TIME);
    }
} scanCallbacks;

LampBluetoothComponent::LampBluetoothComponent(std::__cxx11::string name, LampColor base_color, LampColor shade_color) {
    Serial.printf("Starting Bluetooth Async Client\n");
    NimBLEDevice::init(name);
    NimBLEDevice::setPower(BLE_POWER_LEVEL);


    // Scan for all bluetooth devices and filter the list
    // for lamps by manufacturer ID. If a lamp is found, add
    // it to the found lamps buffer
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks);
    pScan->setInterval(BLE_GAP_ADV_INTERVAL_MS);
    pScan->setWindow(BLE_GAP_SCAN_WINDOW_MS);
    pScan->setActiveScan(true);
    pScan->start(BLE_GAP_SCAN_TIME);

    // Lamps advertise 8 bytes of manufacturer data to share colors
    // 2 bytes: lamp identifier [Manufacturer ID block]
    // 3 bytes: RGB base color (white omitted)
    // 3 bytes: RGB shade color (white omitted)
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(name);
    pAdvertising->enableScanResponse(true);
    std::vector<unsigned char> data{
        static_cast<unsigned char>(BLE_MAGIC_NUMBER & 0xff),
        static_cast<unsigned char>((BLE_MAGIC_NUMBER >> 8) & 0xff),
        static_cast<unsigned char>((base_color.R())),
        static_cast<unsigned char>((base_color.G())),
        static_cast<unsigned char>((base_color.B())),
        static_cast<unsigned char>((shade_color.R())),
        static_cast<unsigned char>((shade_color.G())),
        static_cast<unsigned char>((shade_color.B())),
    };
    pAdvertising->setManufacturerData(data);
    pAdvertising->start();
}

int LampBluetoothComponent::get_found_lamps() {
    return 0;
}
