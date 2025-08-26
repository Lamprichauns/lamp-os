#include "./compositor.hpp"

#include "./behaviors/fade_in.cpp"

namespace lamp {
Compositor::Compositor() {};

void Compositor::begin(std::vector<AnimatedBehavior*> inBehaviors,
                       std::vector<FrameBuffer*> inFrameBuffers) {
  behaviors = inBehaviors;
  frameBuffers = inFrameBuffers;
  for (int i = 0; i < frameBuffers.size(); i++) {
    startupBehaviors.push_back(new FadeInBehavior(
        frameBuffers.at(i), STARTUP_ANIMATION_FRAMES, false, true));
  }
};

void Compositor::tick() {
  if (!behaviorsComputed) {
    if (startupComplete) {
      for (int i = 0; i < behaviors.size(); i++) {
        behaviors.at(i)->control();
        behaviors.at(i)->draw();
      }
    } else {
      for (int i = 0; i < startupBehaviors.size(); i++) {
        startupBehaviors.at(i)->control();
        startupBehaviors.at(i)->draw();
        if (startupBehaviors.at(i)->isLastFrame()) {
          startupComplete = true;
        }
      }
    }

    behaviorsComputed = true;
  }

  if (behaviorsComputed &&
      millis() >= lastDrawTimeMs + MINIMUM_FRAME_DRAW_TIME_MS) {
    lastDrawTimeMs = millis();
    behaviorsComputed = false;
    for (int i = 0; i < frameBuffers.size(); i++) {
      frameBuffers.at(i)->flush();
    }
  };
}
};  // namespace lamp
