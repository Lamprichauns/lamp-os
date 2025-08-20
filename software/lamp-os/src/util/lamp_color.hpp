#ifndef LAMP_UTIL_H
#define LAMP_UTIL_H
#include <Arduino.h>

class LampColor {
private:
    uint32_t color;
public:
    uint8_t r,g,b,w;
    LampColor(uint32_t in_color);
};
#endif