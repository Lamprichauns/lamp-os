#ifndef LAMP_BEHAVIORS_CONFIGURATOR_H
#define LAMP_BEHAVIORS_CONFIGURATOR_H

#include <cstdint>
#include <vector>

#include "../components/network/wifi.hpp"
#include "../config/config_types.hpp"
#include "../core/animated_behavior.hpp"
#include "../util/color.hpp"

#define CONFIGURATOR_WEBSOCKET_TIMEOUT 30000

/**
 * @brief a layer to preview realtime changes from the web
 *        configuration tool
 */
namespace lamp {
class ConfiguratorBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  // the total frame count must be a multiple of the ease frames
  uint32_t easeFrames = 60;
  uint8_t brightness = 100;
  std::vector<Color> colors;
  std::vector<uint8_t> knockoutPixels = std::vector<uint8_t>(50, (uint8_t)100);
  unsigned long lastWebSocketUpdateTimeMs = 0;

  void draw();

  void control();
};
}  // namespace lamp
#endif