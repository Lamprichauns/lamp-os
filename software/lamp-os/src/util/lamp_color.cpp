#include <Arduino.h>
#include "./lamp_color.hpp"

LampColor::LampColor(uint32_t in_color) {
    color = in_color;
}

int LampColor::R() {
    return (color >> 24) & 0xff;
}

int LampColor::G() {
    return (color >> 16) & 0xff;
}

int LampColor::B() {
    return (color >> 8) & 0xff;
}

int LampColor::W() {
    return color & 0xff;
}