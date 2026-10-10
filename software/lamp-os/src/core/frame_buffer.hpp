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

  // Flushes a pixel's colour and the level must hold still before the pixel
  // stops dithering and shows its rounded drive level: 1 s at the compositor's
  // 16 ms floor, longer on slower frames. Shorter and a slow fade snaps between
  // content steps; longer and a static dim scene sparkles after every change.
  static constexpr uint8_t kDitherHoldFrames = 60;

  // Temporally dithered push: a changing pixel between drive levels keeps
  // flush() sending every frame; a pixel held still for kDitherHoldFrames
  // shows its rounded level, so a static scene dedups again. See
  // docs/dev/lamp-framework.md.
  void flush();

 private:
  // Per pixel per channel (w, r, g, b) dither fraction, sized once in begin().
  std::vector<uint8_t> ditherCarry_;
  // Per pixel count of flushes its colour and the level held still, saturating
  // at kDitherHoldFrames.
  std::vector<uint8_t> ditherHold_;
  bool dithering_ = false;
};

}  // namespace lamp
