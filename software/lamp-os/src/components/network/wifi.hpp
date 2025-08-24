#ifndef LAMP_COMPONENTS_NETWORK_WIFI_H
#define LAMP_COMPONENTS_NETWORK_WIFI_H

#include <Arduino.h>

#include <string>
#include <vector>

#include "../../util/color.hpp"

#define UNIVERSE 1

namespace lamp {
class WifiComponent {
 public:
  /**
   * @brief initializer for setup
   * @param [in] name max. 13 character string representing the lamp's name
   */
  void begin(std::string name);

  /**
   * @brief process to call in the main loop
   */
  void tick();

  /**
   * @brief get last Artnet UDP data from the buffer
   * @return vector of shade and base colors
   */
  std::vector<Color> getArtnetData();

  /**
   * @brief get last Artnet UDP frame time
   * @return timestamp of last frame in milliseconds
   */
  unsigned long getLastArtnetFrameTimeMs();
};
}  // namespace lamp

#endif