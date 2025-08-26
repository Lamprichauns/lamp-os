#ifndef LAMP_UTIL_FADE_H
#define LAMP_UTIL_FADE_H
#include <Arduino.h>

#include "./color.hpp"

namespace lamp {
uint8_t ease(uint8_t start, uint8_t end, uint32_t duration,
             uint32_t current_step);

Color fade(Color start, Color end, uint32_t steps, uint32_t currentStep);
}  // namespace lamp
#endif