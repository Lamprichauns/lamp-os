#ifndef LAMP_UTIL_LEVELS_H
#define LAMP_UTIL_LEVELS_H
#include <Arduino.h>

namespace lamp {
int darken(int value, int percentage);

int calculateBrightnessLevel(int value, int percentage);
}  // namespace lamp

#endif