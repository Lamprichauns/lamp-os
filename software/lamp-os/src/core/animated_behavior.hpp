#ifndef LAMP_CORE_ANIMATED_BEHAVIOR_H
#define LAMP_CORE_ANIMATED_BEHAVIOR_H

#include "./frame_buffer.hpp"

namespace lamp {
enum AnimationState {
  // Animation is playing normally
  PLAYING = 1,

  // Animation is about to stop dead in its tracks
  PAUSING = 2,

  // Animation will no longer contribute pixels to the scene and will keep
  // its playhead at the last frame position
  PAUSED = 3,

  // Animation will stop gracefully and let the total frame count continue
  // to contribute to the scene
  STOPPING = 4,

  // Animation will no longer contribute to the scene and it will
  // resume from the beginning of the frame count
  STOPPED = 5
};

/**
 * An Animated behavior is any lamp behavior that will have a side effect on the
 * LEDs. Any animation will run at roughly 30 frames per second
 */
class AnimatedBehavior {
 public:
  FrameBuffer* fb;
  unsigned long frames = 60;
  unsigned long frame = 0;
  unsigned long currentLoop = 0;
  bool immediateControl = false;
  bool autoPlay = false;
  bool homeMode = false;
  AnimationState animationState = STOPPED;

  /**
   * Animated Behavior Base class - integrators implement draw and control
   * functions of their own to control the lamp's LEDs
   * @param [in] inBuffer the frame buffer to interact with
   * @param [in] inFrames the frame duration for the behaviour eg (60 frames ~ 2
   *                      seconds of animation)
   * @param [in] homeMode if true the animation will only play if allowed
   * @param [in] autoPlay if true the animation will begin immediately
   * @param [in] inImmediateControl if true the control block will start on lamp
   *                                startup
   */
  AnimatedBehavior();
  AnimatedBehavior(FrameBuffer* inBuffer, unsigned long inFrames = 60,
                   bool inHomeMode = false, bool inAutoPlay = false,
                   bool inImmediateControl = false);
  virtual ~AnimatedBehavior();
  /**
   * @brief A virtual function to make changes to the frame buffer per frame
   */
  virtual void draw();

  /**
   * @brief A virtual function to do calculations and coordinate animation state
   *        for each animation layer
   */
  virtual void control();

  /**
   * @brief Pause the animation
   */
  void pause();

  /**
   * @brief Stop the animation
   */
  void stop();

  /**
   * @brief Play the animation in a loop
   */
  void play();

  /**
   * @brief Play the animation for one full frame cycle
   */
  void playOnce();

  /**
   * @brief Check for the last frame
   * @return true if the playhead is at the last frame of the animation
   */
  bool isLastFrame();

  /**
   * @brief conclude the draw procedure and advance the internal frame counters
   */
  void nextFrame();
};
}  // namespace lamp

#endif
