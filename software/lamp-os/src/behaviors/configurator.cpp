#include "./configurator.hpp"

#include <cstdint>

#include "../util/color.hpp"
#include "../util/fade.hpp"

namespace lamp {
void ConfiguratorBehavior::draw() {
  for (int i = 0; i < fb->pixelCount; i++) {
    if (frame < easeFrames) {
      fb->buffer[i] = fade(fb->buffer[i], colors[0], easeFrames, frame);
    } else if (frame > (frames - easeFrames)) {
      fb->buffer[i] = fade(colors[0], fb->buffer[i], easeFrames, frame % easeFrames);
    } else {
      fb->buffer[i] = colors[0];
    }
  }

  nextFrame();
};

void ConfiguratorBehavior::control() {
  uint32_t now = millis();
  if (animationState == STOPPED) {
    if (lastWebSocketUpdateTimeMs > 0 &&
        now < lastWebSocketUpdateTimeMs + CONFIGURATOR_WEBSOCKET_TIMEOUT) {
      playOnce();
    }
  }
  if (animationState == PLAYING_ONCE && frame == easeFrames) {
    pause();
  }
  if (animationState == PAUSED && now > lastWebSocketUpdateTimeMs + CONFIGURATOR_WEBSOCKET_TIMEOUT) {
    playOnce();
  }
};
}  // namespace lamp