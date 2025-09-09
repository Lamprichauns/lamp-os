#ifndef LAMP_BEHAVIORS_FADE_OUT_H
#define LAMP_BEHAVIORS_FADE_OUT_H

#include "../core/animated_behavior.hpp"

/**
 * @brief animation to fade to black and reboot
 */
namespace lamp {
class FadeOutBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  bool reboot = false;
  void draw();
  void control();
};
}  // namespace lamp
#endif