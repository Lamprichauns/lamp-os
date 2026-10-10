#pragma once
#include "freertos/FreeRTOS.h"

using SemaphoreHandle_t = void*;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
  static int token;
  return &token;
}
// Takes not yet given back, across every semaphore, so a test can check
// that a path gives back what it took.
inline int g_semaphoresHeld = 0;

inline int xSemaphoreTake(SemaphoreHandle_t, uint32_t) {
  ++g_semaphoresHeld;
  return pdTRUE;
}
inline int xSemaphoreGive(SemaphoreHandle_t) {
  --g_semaphoresHeld;
  return pdTRUE;
}
