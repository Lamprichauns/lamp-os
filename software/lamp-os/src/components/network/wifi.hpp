#ifndef LAMP_COMPONENTS_NETWORK_WIFI_H
#define LAMP_COMPONENTS_NETWORK_WIFI_H

#include <Arduino.h>

#include <string>
#include <vector>

#include "../../config/config.hpp"
#include "../../util/color.hpp"
#define WEBSOCKET_CLEAN_TIME_MS 5000

namespace lamp {
class WifiComponent {
 public:
  Config* config;
  unsigned long lastWebSocketCleanTimeMs = 0;
  std::string doc;
  boolean requiresReboot = false;
  boolean newWebSocketData = false;
  unsigned long lastWebSocketUpdateTimeMs = 0;
  JsonDocument lastWebSocketData;

  WifiComponent();

  /**
   * @brief initializer for setup
   * @param [in] config a reference to a lamp config
   */
  void begin(Config* inConfig);

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

  /**
   * @brief Check if there's new WebSocket data to consume
   * @return true if there's recent data in the buffer
   */
  bool hasWebSocketData();

  /**
   * @brief get last Websocket updates time
   * @return timestamp of last update time in milliseconds
   */
  unsigned long getLastWebSocketUpdateTimeMs();

  /**
   * @brief get the json doc from the buffer
   * @return JsonDocument
   */
  JsonDocument getWebSocketData();
};
}  // namespace lamp
#endif