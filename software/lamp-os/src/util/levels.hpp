#ifndef LAMP_UTIL_LEVELS_H
#define LAMP_UTIL_LEVELS_H

#include <cstdint>

namespace lamp {
uint8_t darken(uint8_t value, uint8_t percentage);

uint8_t calculateBrightnessLevel(uint8_t value, uint8_t percentage);
}  // namespace lamp

#endif