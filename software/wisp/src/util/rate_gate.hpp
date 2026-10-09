// Lets an action run at most once per interval with a trailing edge: a
// request inside the interval is held, and due() releases it once the
// interval has passed, so the last request always lands.

#pragma once

#include <cstdint>

namespace wisp {

class RateGate {
 public:
  explicit RateGate(uint32_t intervalMs) : intervalMs_(intervalMs) {}

  // True when the action should run now; otherwise it is held.
  bool request(uint32_t nowMs) {
    if (!ran_ || nowMs - lastRunMs_ >= intervalMs_) {
      run(nowMs);
      return true;
    }
    pending_ = true;
    return false;
  }

  // True once when a held request's interval has passed.
  bool due(uint32_t nowMs) {
    if (!pending_ || nowMs - lastRunMs_ < intervalMs_) return false;
    run(nowMs);
    return true;
  }

  // Drops any held request; the next request runs immediately.
  void reset() {
    pending_ = false;
    ran_ = false;
  }

 private:
  void run(uint32_t nowMs) {
    lastRunMs_ = nowMs;
    ran_ = true;
    pending_ = false;
  }

  uint32_t intervalMs_;
  uint32_t lastRunMs_ = 0;
  bool ran_ = false;
  bool pending_ = false;
};

}  // namespace wisp
