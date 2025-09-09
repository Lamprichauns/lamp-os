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
void AnimatedBehavior::pause() { animationState = PAUSING; };

void AnimatedBehavior::stop() { animationState = STOPPING; };

void AnimatedBehavior::play() { animationState = PLAYING; };

void AnimatedBehavior::playOnce() { animationState = PLAYING_ONCE; };

bool AnimatedBehavior::isLastFrame() { return (frame == frames - 1); };

void AnimatedBehavior::nextFrame() {
  if (animationState == PAUSING) {
    animationState = PAUSED;
  }

  if (animationState != PAUSED) {
    frame += 1;
  }

  if (frame >= frames) {
    if (animationState == STOPPING || animationState == PLAYING_ONCE) {
      animationState = STOPPED;
    }

    frame = 0;
    currentLoop += 1;
  }
};
}  // namespace lamp
