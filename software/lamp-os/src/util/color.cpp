#include "./color.hpp"

#include <Arduino.h>

namespace lamp {
Color::Color() {
  color = 0;
  r = g = b = w = 0;
}
Color::Color(uint32_t inColor) {
  color = inColor;
  r = (inColor >> 24) & 0xff;
  g = (inColor >> 16) & 0xff;
  b = (inColor >> 8) & 0xff;
  w = inColor & 0xff;
};

Color::Color(uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inW) {
  r = inR;
  g = inG;
  b = inB;
  w = inW;
  color = (uint32_t)((inR << 24) | (inG << 16) | (inB << 8) | (inW));
};
}  // namespace lamp