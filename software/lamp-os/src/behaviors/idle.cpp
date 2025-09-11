
#include "./idle.hpp"

namespace lamp {
void IdleBehavior::draw() {
  for (int i = 0; i < fb->pixelCount; i++) {
    fb->fill(fb->defaultColor);
  }

  nextFrame();
};

void IdleBehavior::control() {};
}  // namespace lamp