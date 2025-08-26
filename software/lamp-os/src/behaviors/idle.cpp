#include "../core/animated_behavior.hpp"
#include "../util/color.hpp"

/**
 * @brief a base layer of the lamp's default color to prevent blackout
 */
namespace lamp {
class IdleBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  void draw() {
    for (int i = 0; i < fb->pixelCount; i++) {
      fb->fill(fb->defaultColor);
    }

    nextFrame();
  };

  void control() {};
};
}  // namespace lamp