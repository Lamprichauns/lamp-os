#include "./bluetooth_pool.hpp"

#include <Arduino.h>

#include <cstdint>
#include <string>

#include "../../util/color.hpp"

namespace lamp {
BluetoothLampRecord::BluetoothLampRecord(std::string inName,
                                         Color inBaseColor,
                                         Color inShadeColor,
                                         uint32_t inLastSeenTimeMs) {
  name = inName;
  baseColor = inBaseColor;
  shadeColor = inShadeColor;
  lastSeenTimeMs = inLastSeenTimeMs;
  acknowledged = false;
};

void BluetoothPool::addLamp(BluetoothLampRecord lamp) {
  if (lampPool.size() < MAX_POOL_SIZE) {
    lampPool.push_back(lamp);
  }
};

void BluetoothPool::addOrUpdateLamp(BluetoothLampRecord lamp) {
  uint32_t timeNow = millis();

  for (int i = 0; i < lampPool.size(); i++) {
    if (lampPool[i].name == lamp.name) {
      lampPool[i].lastSeenTimeMs = timeNow;
      return;
    }
  }

  addLamp(lamp);
};

void BluetoothPool::pruneLamps() {
  uint32_t timeNow = millis();

  for (int i = 0; i < lampPool.size(); i++) {
    if (lampPool[i].lastSeenTimeMs + LAMP_PRUNE_TIME_MS < timeNow) {
      lampPool.erase(lampPool.begin() + i);
    }
  }
}

std::vector<BluetoothLampRecord> BluetoothPool::getLamps() { return lampPool; };

BluetoothStageRecord::BluetoothStageRecord(std::string inName,
                                           String inSsid,
                                           String inPassword,
                                           uint32_t inLastSeenTimeMs) {
  name = inName;
  ssid = inSsid;
  password = inPassword;
  lastSeenTimeMs = inLastSeenTimeMs;
};

void BluetoothPool::addStage(BluetoothStageRecord stage) {
  if (stagePool.size() < MAX_POOL_SIZE) {
    stagePool.push_back(stage);
  }
};

void BluetoothPool::addOrUpdateStage(BluetoothStageRecord stage) {
  uint32_t timeNow = millis();

  for (int i = 0; i < stagePool.size(); i++) {
    if (stagePool[i].name == stage.name) {
      stagePool[i].lastSeenTimeMs = timeNow;
      return;
    }
  }

  addStage(stage);
};

void BluetoothPool::pruneStages() {
  uint32_t timeNow = millis();

  for (int i = 0; i < stagePool.size(); i++) {
    if (stagePool[i].lastSeenTimeMs + STAGE_PRUNE_TIME_MS < timeNow) {
      stagePool.erase(stagePool.begin() + i);
    }
  }
}

std::vector<BluetoothStageRecord> BluetoothPool::getStages() { return stagePool; };
}  // namespace lamp