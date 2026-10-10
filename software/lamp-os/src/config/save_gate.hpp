#pragma once

#include <cstdint>

#include "config/config_codec.hpp"
#include "util/retry_backoff.hpp"

namespace lamp {

// When Config's saves run. A failed save backs off (RetryBackoff) and stays
// pending until one lands. Once a whole web document is stored, every save,
// a second web document's included, holds until the reboot that loads it:
// allows() and due() are false and nothing is pending, so the commit drain
// waits quietly instead of retrying at loop rate, and nothing writes over the
// stored document. A web document whose save failed partway may have left
// part of itself beside the running config, so the running config's save
// goes pending to write it back whole.
class SaveGate {
 public:
  SaveGate(uint32_t baseMs, uint32_t maxMs) : retry_(baseMs, maxMs) {}

  bool allows() const { return !frozen_; }
  bool due(uint32_t nowMs) const { return allows() && retry_.due(nowMs); }
  bool pending() const { return allows() && retry_.pending(); }
  uint32_t waitMs() const { return retry_.waitMs(); }

  void saved() { retry_.succeed(); }
  void failed(uint32_t nowMs) { retry_.fail(nowMs); }
  // A web document's save finished with `result`.
  void rawFinished(config_codec::SaveResult result, uint32_t nowMs) {
    if (result == config_codec::SaveResult::Ok) {
      retry_.succeed();
      frozen_ = true;
    } else if (result == config_codec::SaveResult::WriteFailed) {
      retry_.fail(nowMs);
    }
  }

 private:
  RetryBackoff retry_;
  bool frozen_ = false;
};

}  // namespace lamp
