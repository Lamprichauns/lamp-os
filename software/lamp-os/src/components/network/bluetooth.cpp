#include "NimBLEDevice.h"

void init() {
    NimBLEDevice::init("NimBLE");

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("ABCD"); // advertise the UUID of our service
    pAdvertising->setName("NimBLE"); // advertise the device name
    pAdvertising->start(); 

    
}

void get_found_lamps() {

}