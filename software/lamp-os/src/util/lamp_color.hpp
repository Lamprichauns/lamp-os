#ifndef LAMP_UTIL_H
#define LAMP_UTIL_H
#include <Arduino.h>

class LampColor {
private:
    uint32_t color;
public:
    LampColor(uint32_t in_color);

    int R();
    int G();
    int B();
    int W();
};
#endif