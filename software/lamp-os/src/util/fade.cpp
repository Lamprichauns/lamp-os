#include "./fade.hpp"

#include <cstdint>
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

std::vector<uint16_t> linear = {
    0,    100,  200,  300,  400,  500,  600,  700,  800,  900,  1000, 1100,
    1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300,
    2400, 2500, 2600, 2700, 2800, 2899, 3000, 3100, 3200, 3300, 3400, 3500,
    3600, 3700, 3800, 3900, 4000, 4100, 4200, 4300, 4400, 4500, 4600, 4700,
    4800, 4900, 5000, 5100, 5200, 5300, 5400, 5500, 5600, 5699, 5799, 5900,
    6000, 6100, 6200, 6300, 6400, 6500, 6600, 6700, 6800, 6900, 7000, 7100,
    7200, 7300, 7400, 7500, 7600, 7700, 7800, 7900, 8000, 8100, 8200, 8300,
    8400, 8500, 8600, 8700, 8800, 8900, 9000, 9100, 9200, 9300, 9400, 9500,
    9600, 9700, 9800, 9900, 10000};

uint8_t ease(uint8_t start, uint8_t end, uint32_t duration,
             uint32_t currentStep) {
  uint16_t factor =
      quadEaseInOut[(int)((currentStep * 100 / duration * 100) / 100)];

  return (((end - start) * factor) / 10000) + start;
};

uint8_t easeLinear(uint8_t start, uint8_t end, uint32_t duration,
                   uint32_t currentStep) {
  uint16_t factor = linear[(int)((currentStep * 100 / duration * 100) / 100)];

  return (((end - start) * factor) / 10000) + start;
};

Color fade(Color start, Color end, uint32_t steps, uint32_t currentStep) {
  return Color(
      (start.r == end.r) ? end.r : ease(start.r, end.r, steps, currentStep),
      (start.g == end.g) ? end.g : ease(start.g, end.g, steps, currentStep),
      (start.b == end.b) ? end.b : ease(start.b, end.b, steps, currentStep),
      (start.w == end.w) ? end.w : ease(start.w, end.w, steps, currentStep));
};

Color fadeLinear(Color start, Color end, uint32_t steps, uint32_t currentStep) {
  return Color(
      (start.r == end.r) ? end.r
                         : easeLinear(start.r, end.r, steps, currentStep),
      (start.g == end.g) ? end.g
                         : easeLinear(start.g, end.g, steps, currentStep),
      (start.b == end.b) ? end.b
                         : easeLinear(start.b, end.b, steps, currentStep),
      (start.w == end.w) ? end.w
                         : easeLinear(start.w, end.w, steps, currentStep));
};
}  // namespace lamp
