#pragma once

#include <cstddef>
#include <cstring>

namespace lamp {

// Keys whose blob write failed for want of heap. IDF's writeMultiPageBlob
// leaves the chunk it had just written when its 12-byte bookkeeping
// allocation fails, and no index names it; a rewrite in the same version
// then holds two chunks at one position, and IDF drops the whole blob when
// it reads it or at the next boot. Such a key takes no blob write until the
// next boot, whose scan erases the orphan; the save fails and retries
// meanwhile. Holds kMax keys; past that every blob write is refused, which
// errs safe.
class WriteQuarantine {
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
  bool empty() const { return count_ == 0 && !overflowed_; }

 private:
  char keys_[kMax][kKeyLen + 1] = {};
  size_t count_ = 0;
  bool overflowed_ = false;
};

}  // namespace lamp
