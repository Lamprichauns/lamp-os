#ifndef LAMP_UTIL_LEVELS_H
#define LAMP_UTIL_LEVELS_H

#include <cstdint>

#include "./color.hpp"

namespace lamp {
uint8_t calculateBrightnessLevel(uint8_t value, uint8_t percentage);

Color setColorBrightness(Color inColor, uint8_t percentage);
}  // namespace lamp

#endif