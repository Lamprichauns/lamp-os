#ifndef LAMP_UTIL_FADE_H
#define LAMP_UTIL_FADE_H
#include <Arduino.h>

#include "./color.hpp"

namespace lamp {
/**
 * @brief ease one color byte to another using a quadratic curve
 * @param  [in] start - the start pixel value
 * @param [in] end - the end pixel value
 * @param [in] duration - the duration of the change
 * @param [in] currentStep - the step in the duration of the change
 */
uint8_t ease(uint8_t start, uint8_t end, uint32_t duration,
             uint32_t currentStep);

/**
 * @brief ease one color byte to another using a linear curve
 * @param  [in] start - the start pixel value
 * @param [in] end - the end pixel value
 * @param [in] duration - the duration of the change
 * @param [in] currentStep - the step in the duration of the change
 */
uint8_t easeLinear(uint8_t start, uint8_t end, uint32_t duration,
                   uint32_t currentStep);

/**
 * @brief fade all color bytes to another using a quadratic curve
 * @param  [in] start - the start pixel value
 * @param [in] end - the end pixel value
 * @param [in] steps - the duration of the change
 * @param [in] currentStep - the step in the duration of the change
 */
Color fade(Color start, Color end, uint32_t steps, uint32_t currentStep);

/**
 * @brief fade all color byte to another using a linear curve
 * @param  [in] start - the start pixel value
 * @param [in] end - the end pixel value
 * @param [in] steps - the duration of the change
 * @param [in] currentStep - the step in the duration of the change
 */
Color fadeLinear(Color start, Color end, uint32_t steps, uint32_t currentStep);
}  // namespace lamp
#endif