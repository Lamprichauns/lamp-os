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
      playOnce();
    }
  }
  if (animationState == PLAYING_ONCE && frame == easeFrames) {
    pause();
  }
  if (animationState == PAUSED && now > lastArtnetFrameTimeMs + DMX_ARTNET_TIMEOUT_MS) {
    playOnce();
  }
};

void DmxBehavior::setColor(Color inColor) {
  currentColor = inColor;
};

void DmxBehavior::setLastArtnetFrameTimeMs(uint32_t inLastArtnetFrameTimeMs) {
  lastArtnetFrameTimeMs = inLastArtnetFrameTimeMs;
};
}  // namespace lamp