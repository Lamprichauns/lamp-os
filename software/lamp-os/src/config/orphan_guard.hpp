#pragma once

#include <cstddef>
#include <cstring>

namespace lamp {

// What one NVS blob write did, as a store's writer reports it.
enum class BlobWrite : unsigned char { Ok, OutOfMemory, Failed };

// The keys whose blob write ran out of heap since boot: such a write can
// leave an orphan chunk that a later write of the key would double
// (docs/dev/embedded-heap.md). Holds kMax keys, then every key, until the
// next boot, which forgets them.
class OrphanKeys {
 public:
  static constexpr size_t kMax = 8;
  static constexpr size_t kKeyLen = 15;  // NVS keys are at most 15 bytes

  bool holds(const char* key) const {
    if (overflowed_) return true;
    for (size_t i = 0; i < count_; ++i) {
      if (std::strncmp(keys_[i], key, kKeyLen) == 0) return true;
    }
    return false;
  }
  void add(const char* key) {
    if (holds(key)) return;
    if (count_ == kMax) {
      overflowed_ = true;
      return;
    }
    std::strncpy(keys_[count_], key, kKeyLen);
    keys_[count_][kKeyLen] = '\0';
    ++count_;
  }

 private:
  char keys_[kMax][kKeyLen + 1] = {};
  size_t count_ = 0;
  bool overflowed_ = false;
};

// Writes a blob through `write(key, data, len)`, a store's own NVS write, so
// that a key `orphans` holds never has its value land beside its orphan: each
// write of it is an empty throwaway version, then the value. A write that
// runs out of heap adds its key. True when the value landed. Every store's
// blob writes go through this (NvsConfigStore, and PagedNvs in the native
// suite).
template <class Write>
bool writeBlobPastOrphan(OrphanKeys& orphans, const char* key, const void* data, size_t len,
                         Write&& write) {
  if (orphans.holds(key)) {
    static const char kEmpty = 0;
    if (write(key, &kEmpty, 0) != BlobWrite::Ok) return false;
  }
  const BlobWrite result = write(key, data, len);
  if (result == BlobWrite::OutOfMemory) orphans.add(key);
  return result == BlobWrite::Ok;
}

}  // namespace lamp
