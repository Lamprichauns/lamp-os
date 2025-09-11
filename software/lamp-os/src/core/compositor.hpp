#ifndef LAMP_CORE_COMPOSITOR_H
#define LAMP_CORE_COMPOSITOR_H

#include <vector>

#include "./animated_behavior.hpp"
#include "./frame_buffer.hpp"

#define MINIMUM_FRAME_DRAW_TIME_MS 15
#define STARTUP_ANIMATION_FRAMES 120
#define REBOOT_ANIMATION_FRAMES 120

namespace lamp {
/**
 * @brief the compositor singleton coordinates a list of behaviors in the order
 *        they need to draw
 */
class Compositor {
 public:
  std::vector<AnimatedBehavior*> startupBehaviors;
  std::vector<AnimatedBehavior*> behaviors;
  std::vector<FrameBuffer*> frameBuffers;
  bool startupComplete = false;
  bool behaviorsComputed = false;
  unsigned long lastDrawTimeMs = 0;
  bool homeMode = false;

  Compositor();

  /**
   * @brief initializer
   * @param [in] inBevaviors list of behaviors to execute in priority sequence. last item
   *             is highest priority
   * @param [in] inFrameBuffers of all frame buffers used by the lamp - this helps to
   *             support multi strip lamps
   * @param [in] homeMode if true only behaviors allowedInHomeMode=true will run to reduce distraction at home
   */
  void begin(std::vector<AnimatedBehavior*> inBehaviors, std::vector<FrameBuffer*> inFrameBuffers, bool homeMode = false);

  /**
   * @brief manage building frames and drawing them on the LEDs
   */
  void tick();
};
}  // namespace lamp
#endif