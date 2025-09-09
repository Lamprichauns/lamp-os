#include "./fade_in.hpp"

#include "../util/color.hpp"
#include "../util/fade.hpp"

namespace lamp {
void FadeInBehavior::draw() {
  for (int i = 0; i < fb->pixelCount; i++) {
    fb->buffer[i] = fade(Color(0, 0, 0, 0), fb->defaultColor, frames, frame);
  }

  nextFrame();
};

void FadeInBehavior::control() {
  if (animationState == STOPPED && currentLoop == 0) {
    playOnce();
  }
};
}  // namespace lamp