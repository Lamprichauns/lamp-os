#include <cmath>
#include <cstdint>
#include <vector>

#include "./color.hpp"
#include "./fade.hpp"

namespace lamp {
std::vector<Color> calculateGradient(Color inColorStart, Color inColorEnd,
                                     uint8_t inSteps) {
  std::vector<Color> output;

  for (int i = 0; i < inSteps; i++) {
    output.push_back(fadeLinear(inColorStart, inColorEnd, inSteps, i));
  }

  return output;
};

std::vector<Color> buildGradientWithStops(uint8_t inNumberPixels,
                                          std::vector<Color> inColorStops) {
  uint8_t numberColors = inColorStops.size();
  uint8_t i = 0;
  std::vector<Color> gradient;

  // input color stops are empty
  if (numberColors < 1) {
    return std::vector<Color>{inNumberPixels, Color()};
  }

  // single color - return a uniform pixel buffer
  if (numberColors == 1) {
    return std::vector<Color>{inNumberPixels, inColorStops[0]};
  }

  // two colors - return a single gradient
  if (numberColors == 2) {
    return calculateGradient(inColorStops[0], inColorStops[1], inNumberPixels);
  }

  // multiple colors - use integer math to calculate an even fit for all the
  // stops
  uint8_t steps = floor(inNumberPixels / (numberColors - 1));
  uint8_t remainder = inNumberPixels % (numberColors - 1);
  std::vector<uint8_t> breaks = std::vector<uint8_t>(numberColors - 1, steps);

  if (remainder != 0) {
    for (i = 0; i < numberColors - 1; i++) {
      breaks[i] = breaks[i] + 1;

      remainder--;

      if (remainder == 0) {
        break;
      }
    }
  }

  // with all the breakpoints identified, build the gradients
  std::vector<Color> buf;
  buf.reserve(inNumberPixels);
  for (i = 0; i < breaks.size(); i++) {
    gradient =
        calculateGradient(inColorStops[i], inColorStops[i + 1], breaks[i]);
    buf.insert(buf.end(), gradient.begin(), gradient.end());
  }

  return buf;
};
}  // namespace lamp
