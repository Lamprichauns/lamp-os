#ifndef LAMP_UTIL_H
#define LAMP_UTIL_H
#include <Arduino.h>

/**
 * @brief encapsulate color values as 4 bytes rgbw value and a 32 bit integer
 */
class LampColor {
private:
    uint32_t color;
public:
    uint8_t r,g,b,w;
    LampColor(uint32_t inColor);
    LampColor(
        uint8_t inR,
        uint8_t inG,
        uint8_t inB,
        uint8_t inW
    );
};
#endif