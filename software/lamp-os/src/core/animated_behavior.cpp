#include "./animated_behavior.hpp"

namespace lamp {
AnimatedBehavior::AnimatedBehavior() {}
AnimatedBehavior::AnimatedBehavior(FrameBuffer *inBuffer,
                                   unsigned long inFrames, bool inHomeMode,
                                   bool inAutoPlay, bool inImmediateControl) {
  fb = inBuffer;
  frames = inFrames;
  homeMode = inHomeMode;
  autoPlay = inAutoPlay;
  if (autoPlay) {
    animationState = PLAYING;
  }
  immediateControl = inImmediateControl;
};

AnimatedBehavior::~AnimatedBehavior() {};
void AnimatedBehavior::draw() {};
void AnimatedBehavior::control() {};
void AnimatedBehavior::pause() {
  if (animationState == PLAYING) {
    animationState = PAUSING;
  }
};

void AnimatedBehavior::stop() {
  if (animationState == PLAYING) {
    animationState = STOPPING;
  }
};

void AnimatedBehavior::play() { animationState = PLAYING; };

bool AnimatedBehavior::isLastFrame() { return (frame == frames - 1); };

void AnimatedBehavior::nextFrame() {
  if (animationState == PAUSING) {
    animationState = PAUSED;
  }

  if (animationState != PAUSED || animationState != STOPPED) {
    frame += 1;
  }

  if (frame >= frames) {
    frame = 0;
    currentLoop += 1;

    if (animationState == STOPPING) {
      animationState = STOPPED;
    }
  }
};
}  // namespace lamp
