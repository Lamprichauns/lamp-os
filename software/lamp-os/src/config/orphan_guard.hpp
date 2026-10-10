#pragma once

#include <cstddef>
#include <cstring>

namespace lamp {

// What one NVS blob write did, as a store's writer reports it.
enum class BlobWrite : unsigned char { Ok, OutOfMemory, Failed };

// Keys whose last blob write ran out of heap. IDF's writeMultiPageBlob leaves
// the chunk it had just written when its 12-byte bookkeeping allocation
// fails, named by no index, in the version the write was going to; the next
// write of that key goes to the same version, puts a second chunk at that
// position, and IDF drops the whole blob when it next reads it or at boot. A
// boot with nothing written there since erases the orphan instead. Holds
// kMax keys; past that it holds every key until the boot, which errs safe.
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
  void remove(const char* key) {
    for (size_t i = 0; i < count_; ++i) {
      if (std::strncmp(keys_[i], key, kKeyLen) == 0) {
        std::memcpy(keys_[i], keys_[count_ - 1], kKeyLen + 1);
        --count_;
        return;
      }
    }
  }

 private:
  char keys_[kMax][kKeyLen + 1] = {};
  size_t count_ = 0;
  bool overflowed_ = false;
};

// Writes a blob through `write(key, data, len)`, a store's own NVS write.
// For a key that holds an orphan, an empty throwaway version goes first: it
// takes the orphan's version (and its doubled position), so the real value
// lands in the other version, and erasing the throwaway leaves the orphan
// named by no index for the next boot to erase. A reset between the two
// drops the key at boot, as any orphaned version would. True when the value
// landed. NvsConfigStore and the native suite's PagedNvs both write through
// this.
template <class Write>
bool writeBlobPastOrphan(OrphanKeys& orphans, const char* key, const void* data, size_t len,
                         Write&& write) {
  if (orphans.holds(key)) {
    static const char kEmpty = 0;
    if (write(key, &kEmpty, 0) != BlobWrite::Ok) return false;
  }
  const BlobWrite result = write(key, data, len);
  if (result == BlobWrite::OutOfMemory) {
    orphans.add(key);
    return false;
  }
  if (result != BlobWrite::Ok) return false;
  orphans.remove(key);
  return true;
}

}  // namespace lamp
