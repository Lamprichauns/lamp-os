#include <Arduino.h>
#include "./lamp_color.hpp"

LampColor::LampColor(uint32_t in_color) {
    color = in_color;
    r = (in_color >> 24) & 0xff;
    g = (in_color >> 16) & 0xff;
    b = (in_color >> 8) & 0xff;
    w = in_color & 0xff;
}