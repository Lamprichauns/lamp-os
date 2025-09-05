#include "./dmx.hpp"

#include "../util/color.hpp"

namespace lamp {
void DmxBehavior::draw() {
  fb->fill(currentColor);
  nextFrame();
};

void DmxBehavior::control() {};

void DmxBehavior::setColor(Color inColor) { currentColor = inColor; };

void DmxBehavior::setLastArtnetFrameTimeMs(
    unsigned long inLastArtnetFrameTimeMs) {
  lastArtnetFrameTimeMs = inLastArtnetFrameTimeMs;
};
}  // namespace lamp