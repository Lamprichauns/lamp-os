#ifndef LAMP_COMPONENTS_NETWORK_WIFI_H
#define LAMP_COMPONENTS_NETWORK_WIFI_H

#include <Arduino.h>

#define UNIVERSE 1
#define UNIVERSE_COUNT 2

namespace lamp {
    class WifiComponent {
    public:
        /**
         * @brief initializer for setup
         * @param [in] name max. 13 character string representing the lamp's name
         */
        void begin(std::__cxx11::string name);

        /**
         * @brief process to call in the main loop
         */
        void tick();
    };
}
#endif