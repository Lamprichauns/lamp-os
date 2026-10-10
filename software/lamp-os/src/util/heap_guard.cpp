#include "util/heap_guard.hpp"

#include <esp_heap_caps.h>

#include <new>

namespace lamp {

bool heapCanSpare(size_t bytes) {
  return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) >= bytes + kHeapHeadroom;
}

bool heapHasFree(size_t bytes) {
  return heap_caps_get_free_size(MALLOC_CAP_8BIT) >= bytes + kHeapHeadroom;
}

std::unique_ptr<char[]> allocateSpare(size_t bytes) {
  if (!heapCanSpare(bytes)) return nullptr;
  return std::unique_ptr<char[]>(new (std::nothrow) char[bytes]);
}

std::unique_ptr<char[]> serializeGuarded(const JsonDocument& doc, size_t& len) {
  len = 0;
  if (doc.overflowed()) return nullptr;
  const size_t n = measureJson(doc);
  auto buf = allocateSpare(n + 1);
  if (!buf) return nullptr;
  len = serializeJson(doc, buf.get(), n + 1);
  return buf;
}

}  // namespace lamp
