#pragma once

#include <cstdint>

namespace lamp {

// Spacing for retrying a failed save: the first retry waits `baseMs`, each
// further failure doubles the wait up to `maxMs`, and a success resets it. A
// failure path that consults due() can never spin at loop rate. Clock-only,
// millis() wraparound safe.
class RetryBackoff {
 public:
  RetryBackoff(uint32_t baseMs, uint32_t maxMs) : baseMs_(baseMs), maxMs_(maxMs) {}

  bool pending() const { return waitMs_ != 0; }
  bool due(uint32_t nowMs) const {
    return waitMs_ == 0 || nowMs - failedAtMs_ >= waitMs_;
  }
  void fail(uint32_t nowMs) {
    waitMs_ = waitMs_ == 0 ? baseMs_ : (waitMs_ >= maxMs_ / 2 ? maxMs_ : waitMs_ * 2);
    failedAtMs_ = nowMs;
  }
  void succeed() { waitMs_ = 0; }
  uint32_t waitMs() const { return waitMs_; }

 private:
  uint32_t baseMs_;
  uint32_t maxMs_;
  uint32_t waitMs_ = 0;
  uint32_t failedAtMs_ = 0;
};

}  // namespace lamp
