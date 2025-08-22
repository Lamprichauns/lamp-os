#include "./lamp_pool.hpp"

LampBluetoothRecord::LampBluetoothRecord(
    std::__cxx11::string inName,
    LampColor inBaseColor,
    LampColor inShadeColor,
    unsigned long inLastSeenTimeMs
){
    name = inName;
    baseColor = inBaseColor;
    shadeColor = inShadeColor;
    lastSeenTimeMs = inLastSeenTimeMs;
    acknowledged = false;
};

void LampBluetoothPool::addLamp(LampBluetoothRecord lamp) {
    if (pool.size() < MAX_POOL_SIZE) {
        pool.push_back(lamp);
    }
};

void LampBluetoothPool::addOrUpdateLamp(LampBluetoothRecord lamp) {
    for (int i=0; i<pool.size(); i++) {
        if(pool[i].name == lamp.name) {
            pool[i].lastSeenTimeMs = millis();
            return;
        }
    }

    addLamp(lamp);
};

void LampBluetoothPool::listLamps() {
    for(int i=0; i<pool.size(); i++) {
        Serial.printf("List Item Name: %s time found: %d - acknowledged: %d\n", pool[i].name.c_str(), pool[i].lastSeenTimeMs, pool[i].acknowledged);
        Serial.printf("Color base: #%02x%02x%02x\n", pool[i].baseColor.r, pool[i].baseColor.g, pool[i].baseColor.b);
        Serial.printf("Color shade: #%02x%02x%02x\n", pool[i].shadeColor.r, pool[i].shadeColor.g, pool[i].shadeColor.b);
    }
};
