#ifndef LAMP_COMPONENTS_NETWORK_BLUETOOTH_POOL_H
#define LAMP_COMPONENTS_NETWORK_BLUETOOTH_POOL_H

#include <vector>
#include "../../util/color.hpp"

// Max lamp pool size
#define MAX_POOL_SIZE 20

// Prune lamps after 120 seconds of no bluetooth updates
#define PRUNE_TIME_MS 120000

namespace lamp {
    /**
     * @brief Generic record for lamps found over Bluetooth
     */
    class BluetoothRecord {
    public:
        std::__cxx11::string name;
        Color baseColor = Color(0);
        Color shadeColor = Color(0);
        unsigned long lastSeenTimeMs;
        boolean acknowledged;

        BluetoothRecord(
            std::__cxx11::string inName,
            Color inBaseColor,
            Color inShadeColor,
            unsigned long inTimeFoundMs
        );
    };

    /**
     * @brief A storage mechanism for tracking and listing remote lamps
     */
    class BluetoothPool {
    public:
        std::vector<BluetoothRecord> pool;

        /**
         * @brief add a lamp record to the pool
         * @param [in] lamp - the lamp to track
         */
        void addLamp(BluetoothRecord lamp);

        /**
         * @brief scan the pool for the existence of a lamp and add or update the last seen time
         * @param [in] lamp - the lamp to add or update
         */
        void addOrUpdateLamp(BluetoothRecord lamp);

        /**
         * @brief list lamps in the vicinity
         * @return the pool of lamps
         */
        std::vector<BluetoothRecord> getLamps();

        /**
         * @brief go through the pool and remove lamps that haven't been seen for a while.
         *        the lifespan is defined by PRUNE_TIME_MS
         */
        void pruneLamps();
    };
}
#endif