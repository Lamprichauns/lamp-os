
#include "./idle.hpp"

/**
 * @brief a base layer of the lamp's default color to prevent blackout
 */
namespace lamp {
void IdleBehavior::draw() {
  for (int i = 0; i < fb->pixelCount; i++) {
    fb->fill(fb->defaultColor);
  }

  nextFrame();
};

void IdleBehavior::control() {};
}  // namespace lamp