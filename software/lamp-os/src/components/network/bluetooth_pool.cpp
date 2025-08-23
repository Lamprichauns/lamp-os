#include "./bluetooth_pool.hpp"
#include "../../util/color.hpp"

namespace lamp {
    BluetoothRecord::BluetoothRecord(
        std::__cxx11::string inName,
        Color inBaseColor,
        Color inShadeColor,
        unsigned long inLastSeenTimeMs
    ){
        name = inName;
        baseColor = inBaseColor;
        shadeColor = inShadeColor;
        lastSeenTimeMs = inLastSeenTimeMs;
        acknowledged = false;
    };

    void BluetoothPool::addLamp(BluetoothRecord lamp) {
        if (pool.size() < MAX_POOL_SIZE) {
            pool.push_back(lamp);
        }
    };

    void BluetoothPool::addOrUpdateLamp(BluetoothRecord lamp) {
        unsigned long timeNow = millis();

        for (int i=0; i<pool.size(); i++) {
            if(pool[i].name == lamp.name) {
                pool[i].lastSeenTimeMs = timeNow;
                return;
            }
        }

        addLamp(lamp);
    };

    std::vector<BluetoothRecord> BluetoothPool::getLamps() {
        for(int i=0; i<pool.size(); i++) {
            Serial.printf("List Item Name: %s time found: %d - acknowledged: %d\n", pool[i].name.c_str(), pool[i].lastSeenTimeMs, pool[i].acknowledged);
            Serial.printf("Color base: #%02x%02x%02x\n", pool[i].baseColor.r, pool[i].baseColor.g, pool[i].baseColor.b);
            Serial.printf("Color shade: #%02x%02x%02x\n", pool[i].shadeColor.r, pool[i].shadeColor.g, pool[i].shadeColor.b);
        }

        return pool;
    };

    void BluetoothPool::pruneLamps() {
        unsigned long timeNow = millis();

        for(int i=0; i<pool.size(); i++) {
            if(pool[i].lastSeenTimeMs + PRUNE_TIME_MS < timeNow) {
                pool.erase(pool.begin()+i);
            }
        }
    }
}