
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

LampBluetoothRecord::LampBluetoothRecord(
    std::__cxx11::string inName,
    LampColor inBaseColor,
    LampColor inShadeColor,
    unsigned long inTimeFoundMs
){
    name = inName;
    baseColor = inBaseColor;
    shadeColor = inShadeColor;
    timeFoundMs = inTimeFoundMs;
    acknowledged = false;
};

static std::vector<LampBluetoothRecord> pool;

void LampBluetoothPool::addLamp(LampBluetoothRecord lamp) {
    if (pool.size() < MAX_POOL_SIZE) {
        // skip familiar lamp names that are already found
        for(int i=0; i<pool.size(); i++) {
            if(pool[i].name == lamp.name) {
                lamp.timeFoundMs = millis();
                return;
            }
        }

        pool.push_back(lamp);
    }
};

void LampBluetoothPool::listLamps() {
    for(int i=0; i<pool.size(); i++) {
        Serial.printf("List Item Name: %s - time found: %d - acknowledged: %d\n", pool[i].name.c_str(), pool[i].timeFoundMs, pool[i].acknowledged);
    }
};

void LampBluetoothPool::acknowledgeLamp(std::__cxx11::string name) {
    for(int i=0; i<pool.size(); i++) {
        if (pool[i].name == name) {
            pool[i].acknowledged = true;
        }
    }
};

LampBluetoothPool lampBluetoothPool;

class ScanCallbacks : public NimBLEScanCallbacks {
    bool isLamp(const char *data) {
        if(data[0] == (BLE_MAGIC_NUMBER & 0xff) && data[1] == ((BLE_MAGIC_NUMBER >> 8) & 0xff)) {
            return true;
        }

        return false;
    }

    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        if (advertisedDevice->haveName() && advertisedDevice->haveManufacturerData()) {
            const char *data = advertisedDevice->getManufacturerData().c_str();
            if(advertisedDevice->getRSSI() > BLE_MINIMUM_RSSI_VALUE && isLamp(data)) {
                Serial.printf("Found Lamp: %s\n", advertisedDevice->getName().c_str());
                LampBluetoothRecord lamp = LampBluetoothRecord(
                    advertisedDevice->getName(),
                    LampColor(0),
                    LampColor(0),
                    millis()
                );
                lampBluetoothPool.addLamp(lamp);
            }
        }
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.printf("Scan Ended\n");
        lampBluetoothPool.listLamps();
        lampBluetoothPool.acknowledgeLamp("century");
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
        static_cast<unsigned char>(base_color.r),
        static_cast<unsigned char>(base_color.g),
        static_cast<unsigned char>(base_color.b),
        static_cast<unsigned char>(shade_color.r),
        static_cast<unsigned char>(shade_color.g),
        static_cast<unsigned char>(shade_color.b),
    };
    pAdvertising->setManufacturerData(data);
    pAdvertising->setConnectableMode(0);
    pAdvertising->start();
}

std::vector<LampBluetoothRecord> LampBluetoothComponent::get_all_lamps() {
    return pool;
}
