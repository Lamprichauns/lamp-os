#include "./bluetooth_pool.hpp"

#include <string>

#include "../../util/color.hpp"

namespace lamp {
BluetoothRecord::BluetoothRecord(std::string inName, Color inBaseColor,
                                 Color inShadeColor,
                                 unsigned long inLastSeenTimeMs) {
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

  for (int i = 0; i < pool.size(); i++) {
    if (pool[i].name == lamp.name) {
      pool[i].lastSeenTimeMs = timeNow;
      return;
    }
  }

  addLamp(lamp);
};

std::vector<BluetoothRecord> BluetoothPool::getLamps() { return pool; };

void BluetoothPool::pruneLamps() {
  unsigned long timeNow = millis();

  for (int i = 0; i < pool.size(); i++) {
    if (pool[i].lastSeenTimeMs + PRUNE_TIME_MS < timeNow) {
      pool.erase(pool.begin() + i);
    }
  }
}
}  // namespace lamp