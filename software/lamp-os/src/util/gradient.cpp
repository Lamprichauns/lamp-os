#include <Arduino.h>

#include <vector>

#include "./color.hpp"
#include "./fade.hpp"

namespace lamp {
std::vector<Color> create_gradient(Color inColorStart, Color inColorEnd,
                                   uint8_t inSteps) {
  std::vector<Color> output;

  for (int i = 0; i < inSteps; i++) {
    output.push_back(fadeLinear(inColorStart, inColorEnd, inSteps, i));
  }

  return output;
};
}  // namespace lamp
