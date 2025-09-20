#ifndef LAMP_CONFIG_CONFIG_TYPES_H
#define LAMP_CONFIG_CONFIG_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

#include "../util/color.hpp"

namespace lamp {

/**
 * @brief users may selectively dim parts of their lamp to deal with difficult
 *        led positions like the neck of a vase base that may show the pixels
 *        too sharply
 * @property p - the pixel index to dim. indexed from 0
 * @property b - the brightness level as a percentage
 */
class KnockoutPixel {
 public:
  uint8_t p;
  uint8_t b;
};

/**
 * @brief Global lamp settings to control initialization
 * @property name - a name that can be used to identify this lamp. it can be up
 * to 12 characters long
 * @property brightness - global brightness level for the lamp as a percentage
 * @property homeMode - if true it will disable some animations while at home
 * @property homeModeSSID - SSID to detect for home mode activation
 * @property homeModeBrightness - brightness level to use when home mode is active
 * @property password - password to protect lamp API and web access
 */
class LampSettings {
 public:
  std::string name = "standard";
  uint8_t brightness = 100;
  bool homeMode = false;
  std::string homeModeSSID = "";
  uint8_t homeModeBrightness = 80;
  std::string password = "";
};

/**
 * @brief Settings used for the bulb neopixels
 * @property px - the total pixel count
 * @property colors - a list of up to 5 colors to use
 */
class ShadeSettings {
 public:
  uint8_t px = 38;
  std::vector<Color> colors = {Color(0x00, 0x00, 0x00, 0xFF)};
};

/**
 * @brief Settings used for the base neopixels
 * @property px - the total pixel count
 * @property colors - a list of up to 5 colors to use
 * @property knockoutPixels - a list of knockout pixels to profile the lamp base
 * @property ac - the preferred color index in a gradient
 */
class BaseSettings {
 public:
  uint8_t px = 35;
  std::vector<Color> colors = {Color(0x30, 0x07, 0x83, 0x00)};
  std::vector<uint8_t> knockoutPixels = std::vector<uint8_t>(50, (uint8_t)100);
  uint8_t ac = 0;
};
}  // namespace lamp
#endif