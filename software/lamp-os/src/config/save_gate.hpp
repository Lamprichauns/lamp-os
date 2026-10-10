#pragma once

#include <cstdint>

#include "util/retry_backoff.hpp"

namespace lamp {

// When Config's saves run. A failed save backs off (RetryBackoff) and stays
// pending until one lands. Once a whole web document is stored, every save
// holds until the reboot that loads it, quietly: due() is false, so the
// commit drain waits instead of retrying at loop rate. A web document whose
// save failed may have left part of itself beside the running config, so the
// running config's save goes pending to write it back whole.
class SaveGate {
 public:
  SaveGate(uint32_t baseMs, uint32_t maxMs) : retry_(baseMs, maxMs) {}

  bool due(uint32_t nowMs) const { return !frozen_ && retry_.due(nowMs); }
  bool pending() const { return !frozen_ && retry_.pending(); }
  bool frozen() const { return frozen_; }
  uint32_t waitMs() const { return retry_.waitMs(); }

  void saved() { retry_.succeed(); }
  void failed(uint32_t nowMs) { retry_.fail(nowMs); }
  void rawSaved() {
    retry_.succeed();
    frozen_ = true;
  }
  void rawFailed(uint32_t nowMs) { retry_.fail(nowMs); }

 private:
  RetryBackoff retry_;
  bool frozen_ = false;
};

}  // namespace lamp
