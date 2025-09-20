#ifndef LAMP_COMPONENTS_NETWORK_BLUETOOTH_POOL_H
#define LAMP_COMPONENTS_NETWORK_BLUETOOTH_POOL_H

#include <Arduino.h>

#include <cstdint>
#include <string>
#include <vector>

#include "../../util/color.hpp"

// Max lamp pool size
#define MAX_POOL_SIZE 20

// Prune lamps after 120 seconds of no bluetooth updates
#define LAMP_PRUNE_TIME_MS 120000

// Prune stages after 120 seconds of no bluetooth updates
#define STAGE_PRUNE_TIME_MS 120000

namespace lamp {
/**
 * @brief Generic record for lamps found over Bluetooth
 */
class BluetoothLampRecord {
 public:
  std::string name;
  Color baseColor = Color();
  Color shadeColor = Color();
  uint32_t lastSeenTimeMs;
  bool acknowledged = false;

  BluetoothLampRecord(std::string inName, Color inBaseColor, Color inShadeColor, uint32_t inTimeFoundMs);
};

/**
 * @brief Generic record for stages found by Bluetooth
 */
class BluetoothStageRecord {
 public:
  std::string name;
  String ssid;
  String password;
  uint32_t lastSeenTimeMs;

  BluetoothStageRecord(std::string inName, String inSsid, String inPassword, uint32_t inTimeFoundMs);
};

/**
 * @brief A storage mechanism for tracking and listing remote lamps and stages
 */
class BluetoothPool {
 public:
  std::vector<BluetoothLampRecord> lampPool;
  std::vector<BluetoothStageRecord> stagePool;

  /**
   * @brief add a lamp record to the pool
   * @param [in] lamp - the lamp to track
   */
  void addLamp(BluetoothLampRecord lamp);

  /**
   * @brief scan the pool for the existence of a lamp and add or update the last
   * seen time
   * @param [in] lamp - the lamp to add or update
   */
  void addOrUpdateLamp(BluetoothLampRecord lamp);

  /**
   * @brief list lamps in the vicinity
   * @return the pool of lamps
   */
  std::vector<BluetoothLampRecord> getLamps();

  /**
   * @brief go through the pool and remove lamps that haven't been seen for a
   * while. the lifespan is defined by LAMP_PRUNE_TIME_MS
   */
  void pruneLamps();

  /**
   * @brief add a stage record to the pool
   * @param [in] stage - the stage to track
   */
  void addStage(BluetoothStageRecord stage);

  /**
   * @brief scan the pool for the existence of a stage and add or update the last
   * seen time
   * @param [in] stage - the stage to add or update
   */
  void addOrUpdateStage(BluetoothStageRecord stage);

  /**
   * @brief list stages in the vicinity
   * @return the pool of nearby stages
   */
  std::vector<BluetoothStageRecord> getStages();

  /**
   * @brief go through the pool and remove stages that haven't been seen for a
   * while. the lifespan is defined by STAGE_PRUNE_TIME_MS
   */
  void pruneStages();
};
}  // namespace lamp
#endif