#ifndef LAMP_BLUETOOTH_POOL_H
#define LAMP_BLUETOOTH_POOL_H

#include <vector>
#include "../../util/lamp_color.hpp"

// Max lamp pool size
#define MAX_POOL_SIZE 20

/**
 * @brief Generic record for lamps found over Bluetooth
 */
class LampBluetoothRecord {
public:
    std::__cxx11::string name;
    LampColor baseColor = LampColor(0);
    LampColor shadeColor = LampColor(0);
    unsigned long lastSeenTimeMs;
    boolean acknowledged;

    LampBluetoothRecord(
        std::__cxx11::string inName,
        LampColor inBaseColor,
        LampColor inShadeColor,
        unsigned long inTimeFoundMs
    );
};

/**
 * @brief A storage mechanism for tracking and listing remote lamps
 */
class LampBluetoothPool {
public:
    std::vector<LampBluetoothRecord> pool;

    /**
     * @brief add a lamp record to the pool
     * @param [in] lamp - the lamp to track
     */
    void addLamp(LampBluetoothRecord lamp);

    /**
     * @brief scan the pool for the existence of a lamp and add or update the last seen time
     * @param [in] lamp - the lamp to add or update
     */
    void addOrUpdateLamp(LampBluetoothRecord lamp);

    /**
     * @brief list lamps in the vicinity
     * @return the pool of lamps
     */
    void listLamps();
};

#endif