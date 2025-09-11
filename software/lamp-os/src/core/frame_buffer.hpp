#ifndef LAMP_CORE_FRAME_BUFFER_H
#define LAMP_CORE_FRAME_BUFFER_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include <vector>

#include "./util/color.hpp"

namespace lamp {
/**
 * @brief the frame buffer holds a copy of the current scene's pixel
 * colors for all layered draw operations
 */
class FrameBuffer {
 public:
  std::vector<Color> defaultColors;
  uint8_t pixelCount;
  Adafruit_NeoPixel *driver;
  std::vector<Color> buffer;

  FrameBuffer();

  /**
   * @brief Setup initializer
   * @param [in] inDefaultColors The user's default lamp colors {Color, ..., n} n <= 40
   * @param [in] inPixelCount the number of neopixels in use
   * @param [in] inDriver the NeoPixel instance to use
   */
  void begin(std::vector<Color> inDefaultColors, uint8_t inPixelCount, Adafruit_NeoPixel *inDriver);

  /**
   * @brief fill the framebuffer with a single color
   */
  void fill(Color inColor);

  /**
   * @brief send values from buffer to the LED driver
   */
  void flush();
};
}  // namespace lamp
#endif