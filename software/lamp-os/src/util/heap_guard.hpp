#pragma once

#include <ArduinoJson.h>

#include <cstddef>
#include <memory>

namespace lamp {

// Contiguous bytes a one-shot allocation must leave in the largest free block,
// so ESP-NOW, BLE and wifi still find their buffers right after it. See
// docs/dev/embedded-heap.md.
constexpr size_t kHeapHeadroom = 4096;

// True when the largest free 8-bit block holds `bytes` plus kHeapHeadroom.
bool heapCanSpare(size_t bytes);
// True when the free 8-bit heap, in any blocks, holds `bytes` plus
// kHeapHeadroom: for work that takes many small blocks.
bool heapHasFree(size_t bytes);

// A `bytes`-long buffer, or null when heapCanSpare refuses or the nothrow
// allocation fails. Never throws, never grows.
std::unique_ptr<char[]> allocateSpare(size_t bytes);

// `doc` serialized into one exactly-sized, NUL-terminated buffer; `len` gets
// the byte count. Null when the document overflowed while it was built (it
// would serialize truncated) or the heap cannot spare the buffer.
std::unique_ptr<char[]> serializeGuarded(const JsonDocument& doc, size_t& len);

}  // namespace lamp
