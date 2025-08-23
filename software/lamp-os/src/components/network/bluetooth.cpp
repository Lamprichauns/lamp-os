
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
#include "../../util/color.hpp"
#include "./bluetooth_pool.hpp"

namespace lamp
{
  BluetoothPool lampBluetoothPool;

  class ScanCallbacks : public NimBLEScanCallbacks
  {
    bool isLamp(std::__cxx11::string data)
    {
      if (data.length() == 8 && data.at(0) == (BLE_MAGIC_NUMBER & 0xff) && data.at(1) == ((BLE_MAGIC_NUMBER >> 8) & 0xff))
      {
        return true;
      }

      return false;
    };

    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {
      if (advertisedDevice->haveName() && advertisedDevice->haveManufacturerData())
      {
        std::__cxx11::string data = advertisedDevice->getManufacturerData();
        if (advertisedDevice->getRSSI() > BLE_MINIMUM_RSSI_VALUE && isLamp(data))
        {
          BluetoothRecord lamp = BluetoothRecord(
              advertisedDevice->getName(),
              Color(data.at(2), data.at(3), data.at(4), 0),
              Color(data.at(5), data.at(6), data.at(7), 0),
              millis());
          lampBluetoothPool.addOrUpdateLamp(lamp);
        }
      }
    };

    void onScanEnd(const NimBLEScanResults &results, int reason) override
    {
      lampBluetoothPool.pruneLamps();
      NimBLEDevice::getScan()->start(BLE_GAP_SCAN_TIME_MS);
    }
  } scanCallbacks;

  BluetoothComponent::BluetoothComponent() {};

  void BluetoothComponent::begin(std::__cxx11::string name, Color inBaseColor, Color inShadeColor)
  {
    Serial.printf("Starting Bluetooth Async Client\n");
    NimBLEDevice::init(name);
    NimBLEDevice::setPower(BLE_POWER_LEVEL);

    // Scan for all bluetooth devices and filter the list
    // for lamps by manufacturer ID. If a lamp is found, add
    // it to the found lamps buffer
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks);
    pScan->setInterval(BLE_GAP_ADV_INTERVAL_MS);
    pScan->setWindow(BLE_GAP_SCAN_WINDOW_MS);
    pScan->setActiveScan(true);
    pScan->start(BLE_GAP_SCAN_TIME_MS);

    // Lamps advertise 8 bytes of manufacturer data to share colors
    // 2 bytes: lamp identifier [Manufacturer ID block]
    // 3 bytes: RGB base color (white omitted)
    // 3 bytes: RGB shade color (white omitted)
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(name);
    pAdvertising->enableScanResponse(true);
    std::vector<unsigned char> data{
        static_cast<unsigned char>(BLE_MAGIC_NUMBER & 0xff),
        static_cast<unsigned char>((BLE_MAGIC_NUMBER >> 8) & 0xff),
        static_cast<unsigned char>(inBaseColor.r),
        static_cast<unsigned char>(inBaseColor.g),
        static_cast<unsigned char>(inBaseColor.b),
        static_cast<unsigned char>(inShadeColor.r),
        static_cast<unsigned char>(inShadeColor.g),
        static_cast<unsigned char>(inShadeColor.b),
    };
    pAdvertising->setManufacturerData(data);
    pAdvertising->setConnectableMode(0);
    pAdvertising->start();
  };

  std::vector<BluetoothRecord> BluetoothComponent::getLamps()
  {
    return lampBluetoothPool.getLamps();
  };
}