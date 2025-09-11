#ifndef LAMP_BEHAVIORS_FADE_OUT_H
#define LAMP_BEHAVIORS_FADE_OUT_H

#include "../components/network/wifi.hpp"
#include "../core/animated_behavior.hpp"

/**
 * @brief animation to fade to black and reboot
 */
namespace lamp {
class FadeOutBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  WifiComponent* wifi;
  bool reboot = false;
  bool allowedInHomeMode = true;

  void draw();

  void control();

  void setWifiComponent(WifiComponent* inWifi);
};
}  // namespace lamp
#endif