#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "expressions/expression_invocation.hpp"

namespace lamp {

// Drains one cascade's fan-out to every peer a batch at a time, so the frames
// queued in the WiFi driver at once stay bounded however large the crowd.
// Holds the peers and one serialized invocation in fixed storage, no heap.
// Each peer's frame carries the delay that keeps its intended fire time,
// (i + 1) x stagger capped at kMaxDelayMs from the original fire, less the
// time the drain has already taken. A new cascade replaces any remainder of
// the previous one. Loop task only.
class CascadeScheduler {
 public:
  static constexpr size_t kMaxPeers = 50;        // LampRoster::kCapacity, asserted where both are seen
  static constexpr size_t kBatchSize = 8;        // frames queued at once
  static constexpr uint32_t kBatchGapMs = 50;    // airtime for one batch
  static constexpr size_t kMaxPayloadLen = 768;  // a worst-case invocation fits

  // Starts a fan-out of `json` (an invocation serialized with "delayMs":0).
  // False, and nothing pending, when it is longer than kMaxPayloadLen or has
  // no delayMs field.
  bool begin(const char* json, size_t len, uint32_t staggerMs, uint32_t nowMs);
  // Adds the next peer, closest first; ignored past kMaxPeers.
  void addPeer(const uint8_t mac[6]);
  void cancel() { next_ = count_ = 0; }
  bool active() const { return next_ < count_; }
  size_t pending() const { return count_ - next_; }

  // Sends the next batch through `send(mac, payload, len)` when one is due,
  // returning the frames sent. The first batch is due at once.
  template <class Send>
  size_t tick(uint32_t nowMs, Send&& send) {
    if (!active()) return 0;
    if (batchesSent_ > 0 && nowMs - lastBatchMs_ < kBatchGapMs) return 0;
    size_t sent = 0;
    while (sent < kBatchSize && active()) {
      if (setDelay(delayFor(next_, staggerMs_, nowMs - startMs_))) {
        send(peers_[next_].data(), payload_.data(), len_);
        ++sent;
      }
      ++next_;
    }
    ++batchesSent_;
    lastBatchMs_ = nowMs;
    return sent;
  }

  // The delay peer `index` carries when its frame leaves `elapsedMs` after the
  // original fire: the intended (index + 1) x stagger, capped, less elapsed,
  // never negative.
  static uint32_t delayFor(size_t index, uint32_t staggerMs, uint32_t elapsedMs);

 private:
  // Rewrites the delay digits in place; false if the payload would overflow.
  bool setDelay(uint32_t delayMs);

  std::array<std::array<uint8_t, 6>, kMaxPeers> peers_{};
  std::array<char, kMaxPayloadLen + 1> payload_{};
  size_t len_ = 0;
  size_t delayPos_ = 0;
  size_t delayDigits_ = 0;
  size_t count_ = 0;
  size_t next_ = 0;
  uint32_t staggerMs_ = 0;
  uint32_t startMs_ = 0;
  uint32_t lastBatchMs_ = 0;
  uint32_t batchesSent_ = 0;
};

}  // namespace lamp
