#include "./levels.hpp"

namespace lamp {
int darken(int value, int percentage) {
  int p = 100 - ((percentage * 100) / 100);

  return (value * p) / 100;
};

int calculateBrightnessLevel(int value, int percentage) {
  int p = ((percentage * 100) / 100);

  return (value * p) / 100;
};
}