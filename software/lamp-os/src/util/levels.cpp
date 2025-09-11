#include "./levels.hpp"

#include <cstdint>

namespace lamp {
uint8_t darken(uint8_t value, uint8_t percentage) {
  uint8_t p = 100 - ((percentage * 100) / 100);

  return (value * p) / 100;
};

uint8_t calculateBrightnessLevel(uint8_t value, uint8_t percentage) {
  uint8_t p = ((percentage * 100) / 100);

  return (value * p) / 100;
};
}