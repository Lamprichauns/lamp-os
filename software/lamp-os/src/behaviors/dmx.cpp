#include "./dmx.hpp"

#include <cstdint>

#include "../util/color.hpp"
#include "../util/fade.hpp"

namespace lamp {
void DmxBehavior::draw() {
  for (int i = 0; i < fb->pixelCount; i++) {
    if (frame < easeFrames) {
      fb->buffer[i] = fade(fb->buffer[i], currentColor, easeFrames, frame);
    } else if (frame > (frames - easeFrames)) {
      fb->buffer[i] = fade(currentColor, fb->buffer[i], easeFrames, frame % easeFrames);
    } else {
      fb->buffer[i] = currentColor;
    }
  }

  nextFrame();
};

void DmxBehavior::control() {
  uint32_t now = millis();
  if (animationState == STOPPED) {
    if (lastArtnetFrameTimeMs > 0 && now < lastArtnetFrameTimeMs + DMX_ARTNET_TIMEOUT_MS) {
      currentColor = currentDmxColor;  // sample and hold a value from dmx for transition in
      playOnce();
    }
  }

  if (animationState == PLAYING_ONCE && frame == easeFrames * 2) {
    pause();
  }

  if (animationState == PAUSED && now > lastArtnetFrameTimeMs + DMX_ARTNET_TIMEOUT_MS) {
    playOnce();
  }

  if (animationState == PAUSED) {
    // the lamp will now begin a sub animation to smear between what dmx is sending
    if (transitionFrame == 0) {
      // create the ranges
      startColor = currentColor;
      endColor = currentDmxColor;
      transitionFrames = random(DMX_BEHAVIOR_FADE_TIME_MIN_MS, DMX_BEHAVIOR_FADE_TIME_MAX_MS);
    }

    if (transitionFrames > 400) {
      // use a smoother algo for longer transitions to prevent obvious led changes
      currentColor = fadeLinear(startColor, endColor, transitionFrames, transitionFrame);
    } else {
      // use a more exciting algo for short transitions
      currentColor = fade(startColor, endColor, transitionFrames, transitionFrame);
    }

    transitionFrame++;

    if (transitionFrame >= transitionFrames) {
      transitionFrame = 0;
    }
  }
};

void DmxBehavior::setColor(Color inColor) {
  currentDmxColor = inColor;
};

void DmxBehavior::setLastArtnetFrameTimeMs(uint32_t inLastArtnetFrameTimeMs) {
  lastArtnetFrameTimeMs = inLastArtnetFrameTimeMs;
};
}  // namespace lamp