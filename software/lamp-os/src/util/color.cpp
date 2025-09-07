#include "./color.hpp"

#include <Arduino.h>

#include <format>
#include <string>

namespace lamp {
std::string colorToHexString(Color inColor) {
  return std::format("#{:02x}{:02x}{:02x}{:02x}", inColor.r, inColor.g,
                     inColor.b, inColor.w);
};

Color::Color() { r = g = b = w = 0; }

Color::Color(uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inW) {
  r = inR;
  g = inG;
  b = inB;
  w = inW;
};
}  // namespace lamp