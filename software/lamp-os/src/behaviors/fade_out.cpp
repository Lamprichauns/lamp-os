#include "./fade_out.hpp"

#include "../util/color.hpp"
#include "../util/fade.hpp"

namespace lamp {
void FadeOutBehavior::draw() {
  for (int i = 0; i < fb->pixelCount; i++) {
    fb->buffer[i] = fade(fb->buffer[i], Color(0, 0, 0, 0), frames, frame);
  }

  nextFrame();
};

void FadeOutBehavior::control() {
  if (animationState == STOPPED && reboot) {
    playOnce();
  }
  if (reboot & isLastFrame()) {
    ESP.restart();
  }
};
}  // namespace lamp