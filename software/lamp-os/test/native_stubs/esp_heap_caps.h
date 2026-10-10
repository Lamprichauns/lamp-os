#pragma once
// Native fake for the ESP-IDF heap-caps queries. Tests set
// g_mock_largest_free_block and g_mock_free_size to drive the low-heap paths.
#include <cstddef>
#include <cstdint>

#define MALLOC_CAP_8BIT (1 << 2)

inline size_t g_mock_largest_free_block = 1 << 20;

inline size_t heap_caps_get_largest_free_block(uint32_t) {
  return g_mock_largest_free_block;
}

inline size_t g_mock_free_size = 1 << 20;

inline size_t heap_caps_get_free_size(uint32_t) { return g_mock_free_size; }
