#ifndef LAMP_BEHAVIORS_CONFIGURATOR_H
#define LAMP_BEHAVIORS_CONFIGURATOR_H

#include "../components/network/wifi.hpp"
#include "../core/animated_behavior.hpp"
#include "../util/color.hpp"

/**
 * @brief a layer to preview realtime changes from the web
 *        configuration tool
 */
namespace lamp {
class ConfiguratorBehavior : public AnimatedBehavior {
  using AnimatedBehavior::AnimatedBehavior;

 public:
  void draw();

  void control();
};
}  // namespace lamp
#endif