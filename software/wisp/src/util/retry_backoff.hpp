// Retry schedule for an attempt that keeps failing: the first retry waits
// kInitialMs, each further failure doubles the wait up to kMaxMs. Wrap-safe
// across the millis() rollover.

#pragma once

#include <cstdint>

namespace wisp {

class RetryBackoff {
 public:
  static constexpr uint32_t kInitialMs = 10000;
  static constexpr uint32_t kMaxMs = 300000;

  // Schedules an attempt due at once and restarts the waits.
  void reset(uint32_t nowMs) {
    delayMs_ = kInitialMs;
    nextMs_ = nowMs;
  }

  // Valid after reset() or failed().
  bool due(uint32_t nowMs) const {
    return static_cast<int32_t>(nowMs - nextMs_) >= 0;
  }

  // An attempt failed at nowMs: the next is due after the current wait, which
  // is returned, and the wait after that doubles.
  uint32_t failed(uint32_t nowMs) {
    const uint32_t wait = delayMs_;
    nextMs_ = nowMs + wait;
    delayMs_ = delayMs_ >= kMaxMs / 2 ? kMaxMs : delayMs_ * 2;
    return wait;
  }

 private:
  uint32_t delayMs_ = kInitialMs;
  uint32_t nextMs_ = 0;
};

}  // namespace wisp
