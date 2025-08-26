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
  Color currentColor = Color(0);

  void draw() {
    fb->fill(currentColor);
    nextFrame();
  };

  void control() {};

  void setColor(Color inColor) { currentColor = inColor; }
};
}  // namespace lamp