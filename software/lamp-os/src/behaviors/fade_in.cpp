#include "../core/animated_behavior.hpp"
#include "../util/color.hpp"
#include "../util/fade.hpp"

/**
 * @brief animation to fade from black to the lamp default color
 */
namespace lamp {
class FadeInBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  void draw() {
    for (int i = 0; i < fb->pixelCount; i++) {
      fb->buffer[i] = fade(Color(0, 0, 0, 0), fb->defaultColor, frames, frame);
    }

    nextFrame();
  };

  void control() {
    if (isLastFrame()) {
      stop();
    }
  };
};
}  // namespace lamp