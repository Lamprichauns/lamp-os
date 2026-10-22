#pragma once
// Minimal fake for native tests that pull FastRng (src/util/fast_rng.hpp
// includes esp_system.h for esp_random()). Fixed value keeps seeded streams
// deterministic under test.
#include <cstdint>

inline uint32_t esp_random() { return 0xA3C59AC3u; }

// Same ordinal values as ESP-IDF's esp_system.h.
typedef enum {
  ESP_RST_UNKNOWN,
  ESP_RST_POWERON,
  ESP_RST_EXT,
  ESP_RST_SW,
  ESP_RST_PANIC,
  ESP_RST_INT_WDT,
  ESP_RST_TASK_WDT,
  ESP_RST_WDT,
  ESP_RST_DEEPSLEEP,
  ESP_RST_BROWNOUT,
} esp_reset_reason_t;
