#pragma once

#include <string>

#include "config_store.hpp"

namespace lamp {

// NVS-backed ConfigStore: all keys live in the "lamp" namespace. Talks to the
// nvs_* API directly: Preferences logs its own failures through a path that
// faults on a fragmented heap, and a full NVS must be a returned 0, not a
// crash. Each failure logs one line. Must be driven from Core 1 (NVS is not
// Core-0-safe).
class NvsConfigStore : public ConfigStore {
 public:
  std::string read(const char* key, const char* defaultValue) override;
  size_t write(const char* key, const char* value) override;
  size_t readBlob(const char* key, void* out, size_t cap) override;
  size_t writeBlob(const char* key, const void* data, size_t len) override;
  bool erase(const char* key) override;
  bool clear() override;

  // Boot gate, before anything opens NVS: initializes the flash partition and,
  // when it cannot be initialized, erases and re-initializes it. Every stored
  // key is lost then, but the lamp boots with BLE instead of needing a
  // reflash. Returns true when NVS is usable.
  static bool initFlash();
};

}  // namespace lamp
