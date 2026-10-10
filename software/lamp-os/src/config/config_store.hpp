#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>

namespace lamp {

// Persistence seam for Config. Owns the key/value backing (NVS on device,
// in-memory in tests), so Config carries no platform dependency and its
// serialization/policy logic can be exercised in the native suite.
//
// Reads return defaultValue (or 0 bytes) when the key is absent or the backing
// is unavailable. A write returns bytes stored, 0 when nothing was written (the
// backing could not be opened, or is full); a failed write never faults. All
// access is single-writer on Core 1 (NVS is not Core-0-safe); the seam adds no
// locking of its own.
class ConfigStore {
 public:
  virtual ~ConfigStore() = default;
  virtual std::string read(const char* key, const char* defaultValue) = 0;
  // Length of the string copied into `out` with its NUL; 0 when absent or
  // longer than `cap` - 1. Takes no heap.
  virtual size_t readString(const char* key, char* out, size_t cap) = 0;
  virtual size_t write(const char* key, const char* value) = 0;
  // Bytes copied into `out`; 0 when absent or longer than `cap`.
  virtual size_t readBlob(const char* key, void* out, size_t cap) = 0;
  virtual size_t writeBlob(const char* key, const void* data, size_t len) = 0;
  // True when the key holds a value of any type, readable or not.
  virtual bool has(const char* key) = 0;
  // True when the key is gone afterwards (absent counts).
  virtual bool erase(const char* key) = 0;
  // Wipe the whole namespace (factory reset). Returns true on success.
  virtual bool clear() = 0;
};

// In-memory ConfigStore for the native suite (no Arduino/flash). The second
// implementation that makes ConfigStore a test seam rather than speculative
// indirection (see docs/dev/code-smells.md). `failWrites` makes every write
// return 0, the way a full NVS does; `writesBeforeCrash` lets that many writes
// and erases land and fails the rest, the way a reset mid-save leaves NVS.
class InMemoryConfigStore : public ConfigStore {
 public:
  bool failWrites = false;
  size_t writeCount = 0;
  size_t writesBeforeCrash = SIZE_MAX;

  std::string read(const char* key, const char* defaultValue) override {
    auto it = map_.find(key);
    return it == map_.end() ? std::string(defaultValue) : it->second;
  }
  size_t readString(const char* key, char* out, size_t cap) override {
    auto it = map_.find(key);
    if (it == map_.end() || it->second.size() + 1 > cap) return 0;
    std::memcpy(out, it->second.c_str(), it->second.size() + 1);
    return it->second.size();
  }
  size_t write(const char* key, const char* value) override {
    return writeBlob(key, value, std::strlen(value));
  }
  size_t readBlob(const char* key, void* out, size_t cap) override {
    auto it = map_.find(key);
    if (it == map_.end() || it->second.size() > cap) return 0;
    std::memcpy(out, it->second.data(), it->second.size());
    return it->second.size();
  }
  size_t writeBlob(const char* key, const void* data, size_t len) override {
    if (crashed()) return 0;
    ++writeCount;
    if (failWrites) return 0;
    map_[key].assign(static_cast<const char*>(data), len);
    return len;
  }
  bool erase(const char* key) override {
    if (crashed() || failWrites) return false;
    map_.erase(key);
    return true;
  }
  bool clear() override {
    map_.clear();
    return true;
  }
  bool has(const char* key) override { return map_.count(key) != 0; }

 private:
  bool crashed() {
    if (writesBeforeCrash == 0) return true;
    if (writesBeforeCrash != SIZE_MAX) --writesBeforeCrash;
    return false;
  }
  std::map<std::string, std::string> map_;
};

}  // namespace lamp
