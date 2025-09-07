#ifndef LAMP_BEHAVIORS_DMX_H
#define LAMP_BEHAVIORS_DMX_H

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
  unsigned long lastArtnetFrameTimeMs = 0;

  void draw();

  void control();

  void setColor(Color inColor);

  void setLastArtnetFrameTimeMs(unsigned long inLastArtnetFrameTimeMs);
};
}  // namespace lamp
#endif