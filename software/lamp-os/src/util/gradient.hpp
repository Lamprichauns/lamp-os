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
std::vector<Color> calculateGradient(Color inColorStart, Color inColorEnd,
                                     uint8_t inSteps);

/**
 * @brief given a list of colors, evenly fill a pixel buffer with a smooth
 * gradient
 * @param [in] inNumberPixels the total Neopixel count to spread the gradient
 * @param [in] inColorSteps up to 5 user colors to fade between
 */
std::vector<Color> buildGradientWithStops(uint8_t inNumberPixels,
                                          std::vector<Color> inColorStops);
}  // namespace lamp

#endif
