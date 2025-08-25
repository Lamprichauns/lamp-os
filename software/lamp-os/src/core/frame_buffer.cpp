#include "./frame_buffer.hpp"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include <vector>

#include "./util/color.hpp"

namespace lamp {
FrameBuffer::FrameBuffer() {};

void FrameBuffer::begin(Color inDefaultColor, uint8_t inPixelCount,
                        Adafruit_NeoPixel *inDriver) {
  defaultColor = inDefaultColor;
  pixelCount = inPixelCount;
  buffer = std::vector<Color>(inPixelCount);
  driver = inDriver;
  driver->begin();
  fill(inDefaultColor);
};

void FrameBuffer::fill(Color inColor) {
  for (int i = 0; i < pixelCount; i++) {
    buffer.at(i) = inColor;
  }
};

void FrameBuffer::flush() {
  for (int i = 0; i < pixelCount; i++) {
    driver->setPixelColor(i,
                          (uint32_t)((buffer[i].w << 24) | (buffer[i].r << 16) |
                                     (buffer[i].g << 8) | (buffer[i].b)));
  }

  if (driver->canShow()) {
    driver->show();
  }
};
}  // namespace lamp