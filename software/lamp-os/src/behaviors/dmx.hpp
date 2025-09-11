#ifndef LAMP_BEHAVIORS_DMX_H
#define LAMP_BEHAVIORS_DMX_H

#include <cstdint>

#include "../components/network/wifi.hpp"
#include "../core/animated_behavior.hpp"
#include "../util/color.hpp"

/**
 * @brief a layer to display dmx data from Artnet
 */
namespace lamp {
class DmxBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  Color currentColor = Color();
  uint32_t lastArtnetFrameTimeMs = 0;

  void draw();

  void control();

  void setColor(Color inColor);

  void setLastArtnetFrameTimeMs(uint32_t inLastArtnetFrameTimeMs);
};
}  // namespace lamp
#endif