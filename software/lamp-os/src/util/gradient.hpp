#ifndef LAMP_UTIL_GRADIENT_H
#define LAMP_UTIL_GRADIENT_H
#include <Arduino.h>

#include <vector>

#include "./color.hpp"
#include "./fade.hpp"

namespace lamp {
/**
 * @brief make a smooth gradient from one color to another
 * @param [in] inColorStart - the start color
 * @param [in] inColorEnd - the end color
 * @param [in] inSteps - the number of pixels the gradients should span
 * @return a vector of colors
 */
std::vector<Color> create_gradient(Color inColorStart, Color inColorEnd,
                                   uint8_t inSteps);
}  // namespace lamp

#endif
