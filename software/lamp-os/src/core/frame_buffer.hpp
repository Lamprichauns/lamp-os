#pragma once

#include <Adafruit_NeoPixel.h>

#include <cstdint>
#include <vector>

#include "util/color.hpp"

namespace lamp {

// Geometry-only strip segment. Palettes live in Config.
struct StripSegment {
  Adafruit_NeoPixel* driver;
  const char* name;
  uint16_t offset;     // start index in the logical buffer
  uint16_t pixelCount;
  bool reversed = false;
};

class FrameBuffer {
 public:
  std::vector<Color> defaultColors;
  std::vector<Color> previousBuffer;
  // Strip level 0..255 folded into flush() with gamma. Drivers stay at the
  // NeoPixel default (no scaling); never call setBrightness on a driver.
  uint8_t level = 255;
  uint8_t previousLevel = 255;
  uint8_t pixelCount = 0;
  std::vector<Color> buffer;
  std::vector<StripSegment> segments;

  FrameBuffer();

  // Primary form: sizes buffer to Σ seg.pixelCount; initializes every segment driver.
  // Σ pixelCount must fit in uint8_t (enforced upstream in validateHwConfig/config_codec).
  void begin(std::vector<Color> inDefaultColors, std::vector<StripSegment> inSegments);

  void begin(std::vector<Color> inDefaultColors, uint8_t inPixelCount, Adafruit_NeoPixel* inDriver);

  // Temporally dithered push: a fractional channel keeps flush() sending every
  // frame until every channel lands on a whole drive level. See
  // docs/dev/lamp-framework.md.
  void flush();

 private:
  // Per pixel per channel (w, r, g, b) dither fraction, sized once in begin().
  std::vector<uint8_t> ditherCarry_;
  bool dithering_ = false;
};

}  // namespace lamp
