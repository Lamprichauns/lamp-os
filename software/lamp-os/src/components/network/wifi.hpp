#ifndef LAMP_COMPONENTS_NETWORK_WIFI_H
#define LAMP_COMPONENTS_NETWORK_WIFI_H

#include <Arduino.h>

#include <string>
#include <vector>

#include "../../util/color.hpp"

#define ARTNET_NETWORK_SCAN_MS 10000

namespace lamp {

/**
 * The async states of the wifi station connection
 */
enum StaState {
  // not connected to a router
  DISCONNECTED = 1,

  // testing for wifi connnectivity
  CONNECTING,

  // successfully connected to wifi
  CONNECTED
};

class WifiComponent {
 public:
  WifiComponent();

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
   * @brief Check if there's new Artnet data to consume to save unnecessary
   *        LED updates
   * @return true if there's recent data in the buffer
   */
  bool hasArtnetData();

  /**
   * @brief get last Artnet UDP data from the buffer
   * @return vector of shade[0] and base[1] colors
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