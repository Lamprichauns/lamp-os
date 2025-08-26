#include "./fade.hpp"

#include <Arduino.h>

#include <vector>

#include "./color.hpp"

namespace lamp {
std::vector<uint16_t> quadEaseInOut = {
    0,    2,    8,    18,   32,   50,   72,   98,   128,  162,  200,  242,
    288,  338,  392,  450,  512,  578,  648,  722,  800,  881,  968,  1058,
    1152, 1250, 1352, 1458, 1568, 1682, 1800, 1922, 2048, 2178, 2312, 2449,
    2592, 2738, 2888, 3042, 3200, 3361, 3527, 3697, 3872, 4050, 4232, 4418,
    4608, 4802, 5000, 5198, 5392, 5582, 5768, 5950, 6128, 6301, 6471, 6638,
    6800, 6958, 7111, 7261, 7408, 7549, 7688, 7822, 7952, 8077, 8199, 8318,
    8432, 8542, 8648, 8750, 8848, 8942, 9032, 9118, 9200, 9278, 9352, 9422,
    9488, 9550, 9608, 9661, 9712, 9758, 9800, 9838, 9872, 9902, 9927, 9949,
    9968, 9982, 9992, 9998, 10000};

uint8_t ease(uint8_t start, uint8_t end, uint32_t duration,
             uint32_t current_step) {
  uint16_t factor =
      quadEaseInOut.at((int)((current_step * 100 / duration * 100) / 100));

  return ((end - start) * factor / 10000) + start;
};

Color fade(Color start, Color end, uint32_t steps, uint32_t currentStep) {
  return Color(ease(start.r, end.r, steps, currentStep),
               ease(start.g, end.g, steps, currentStep),
               ease(start.b, end.b, steps, currentStep),
               ease(start.w, end.w, steps, currentStep));
};
}  // namespace lamp
