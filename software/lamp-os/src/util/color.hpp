#ifndef LAMP_UTIL_COLOR_H
#define LAMP_UTIL_COLOR_H
#include <Arduino.h>

#include <string>

namespace lamp {
/**
 * @brief encapsulate color values as 4 bytes rgbw value and a 32 bit integer
 */
class Color {
 public:
  uint8_t r, g, b, w;
  Color();
  Color(uint32_t inColor);
  Color(uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inW);
};

std::string colorToHexString(Color inColor);
}  // namespace lamp
#endif