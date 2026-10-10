#include "frame_buffer.hpp"

#include <Adafruit_NeoPixel.h>

#include <cstdint>
#include <vector>

#include "util/color.hpp"
#include "util/dither.hpp"

namespace lamp {

FrameBuffer::FrameBuffer() {}

void FrameBuffer::begin(std::vector<Color> inDefaultColors, std::vector<StripSegment> inSegments) {
  defaultColors = std::move(inDefaultColors);
  segments = std::move(inSegments);
  uint16_t total = 0;
  for (const auto& seg : segments) {
    total += seg.pixelCount;
  }
  pixelCount = static_cast<uint8_t>(total);
  buffer = std::vector<Color>(pixelCount);
  // Staggered seeds keep a uniform fractional level from pulsing every pixel
  // on the same frame.
  ditherCarry_.resize(static_cast<size_t>(pixelCount) * 4);
  for (size_t k = 0; k < ditherCarry_.size(); k++) ditherCarry_[k] = static_cast<uint8_t>(k * 157);
  for (auto& seg : segments) {
    seg.driver->begin();
    seg.driver->fill(0);
    seg.driver->show();
  }
}

void FrameBuffer::begin(std::vector<Color> inDefaultColors, uint8_t inPixelCount,
                        Adafruit_NeoPixel* inDriver) {
  begin(std::move(inDefaultColors), {{inDriver, "", 0, inPixelCount}});
}

void FrameBuffer::flush() {
  if (segments.empty()) return;
  if (!dithering_ && buffer == previousBuffer && level == previousLevel) return;
  bool allShown = true;
  bool fractional = false;
  for (const auto& seg : segments) {
    for (uint16_t i = 0; i < seg.pixelCount; i++) {
      const uint16_t idx = seg.offset + (seg.reversed ? (seg.pixelCount - 1 - i) : i);
      const Color& c = buffer[idx];
      const uint8_t channels[4] = {c.w, c.r, c.g, c.b};
      uint32_t packed = 0;
      for (size_t ch = 0; ch < 4; ch++) {
        const uint16_t target = gammaLevel88(channels[ch], level);
        fractional |= (target & 0xFF) != 0;
        packed = (packed << 8) | ditherStep(target, ditherCarry_[idx * 4u + ch]);
      }
      seg.driver->setPixelColor(i, packed);
    }
    if (seg.driver->canShow()) {
      seg.driver->show();
    } else {
      allShown = false;
    }
  }
  dithering_ = fractional;
  // A skipped segment retransmits on the next frame.
  if (allShown) {
    previousBuffer = buffer;
    previousLevel = level;
  }
}

}  // namespace lamp
