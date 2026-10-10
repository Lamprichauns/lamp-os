#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "expressions/expression_invocation.hpp"

namespace lamp {

// Drains cascade fan-outs to every peer a batch at a time, so the frames a
// cascade queues in the WiFi driver at once stay bounded however large the
// crowd. Holds up to kMaxDrains fan-outs (their peers and one serialized
// invocation each) in fixed storage, no heap; they drain in order. A cascade
// arriving while every slot is taken replaces the one waiting last, never the
// one draining. Each peer's frame carries the delay that keeps its intended
// fire time, (i + 1) x stagger capped at kMaxDelayMs from that cascade's own
// fire, less the time already elapsed. Loop task only.
class CascadeScheduler {
 public:
  static constexpr size_t kMaxPeers = 50;        // LampRoster::kCapacity, asserted where both are seen
  static constexpr size_t kMaxDrains = 2;        // fan-outs held at once
  static constexpr size_t kBatchSize = 8;        // frames sent per pass at most
  // Outlasts a command's resend copies (MeshLink: 2 more, 40 ms apart), so
  // one batch never overwrites the previous batch's pending resends in the
  // 10-slot resend ring.
  static constexpr uint32_t kBatchGapMs = 100;
  static constexpr size_t kMaxPayloadLen = 768;  // a worst-case invocation fits

  enum class Begin : uint8_t { Queued, Replaced, TooLong, NoDelayField };

  // Queues a fan-out of `json` (an invocation serialized with "delayMs":0)
  // and makes it the target of addPeer. TooLong or NoDelayField hold nothing
  // new.
  Begin begin(const char* json, size_t len, uint32_t staggerMs, uint32_t nowMs);
  // Adds the next peer to the fan-out begin() just queued, closest first;
  // ignored past kMaxPeers.
  void addPeer(const uint8_t mac[6]);
  void cancel();
  bool active() const { return count_ > 0; }
  // Peers not yet sent across every held fan-out.
  size_t pending() const;
  // Frames dropped because a delay rewrite would overflow the payload.
  uint32_t overflowDrops() const { return overflowDrops_; }

  // Sends from the oldest fan-out through `send(mac, payload, len)`, which
  // returns false when the driver cannot take the frame now; that frame is
  // retried on a later pass. Returns the frames sent.
  template <class Send>
  size_t tick(uint32_t nowMs, Send&& send) {
    if (!active()) return 0;
    Drain& d = drains_[head_];
    if (d.batchesSent > 0 && nowMs - d.lastBatchMs < kBatchGapMs) return 0;
    size_t sent = 0;
    while (sent < kBatchSize && d.next < d.count) {
      if (!setDelay(d, delayFor(d.next, d.staggerMs, nowMs - d.startMs))) {
        ++overflowDrops_;
        ++d.next;
        continue;
      }
      if (!send(d.peers[d.next].data(), d.payload.data(), d.len)) break;
      ++sent;
      ++d.next;
    }
    ++d.batchesSent;
    d.lastBatchMs = nowMs;
    if (d.next >= d.count) pop();
    return sent;
  }

  // tick() for a sender that never refuses, gated on `ready()`: a frame waits
  // for a later pass while the driver has no room.
  template <class Ready, class Send>
  size_t tickGated(uint32_t nowMs, Ready&& ready, Send&& send) {
    return tick(nowMs, [&](const uint8_t* mac, const char* payload, size_t len) {
      if (!ready()) return false;
      send(mac, payload, len);
      return true;
    });
  }

  // The delay peer `index` carries when its frame leaves `elapsedMs` after the
  // original fire: the intended (index + 1) x stagger, capped, less elapsed,
  // never negative.
  static uint32_t delayFor(size_t index, uint32_t staggerMs, uint32_t elapsedMs);

 private:
  struct Drain {
    std::array<std::array<uint8_t, 6>, kMaxPeers> peers{};
    std::array<char, kMaxPayloadLen + 1> payload{};
    size_t len = 0;
    size_t delayPos = 0;
    size_t delayDigits = 0;
    size_t count = 0;
    size_t next = 0;
    uint32_t staggerMs = 0;
    uint32_t startMs = 0;
    uint32_t lastBatchMs = 0;
    uint32_t batchesSent = 0;
  };
  // Rewrites the delay digits in place; false if the payload would overflow.
  static bool setDelay(Drain& d, uint32_t delayMs);
  void pop();

  std::array<Drain, kMaxDrains> drains_{};
  size_t head_ = 0;
  size_t count_ = 0;
  size_t filling_ = 0;  // the fan-out addPeer feeds
  bool fillingValid_ = false;
  uint32_t overflowDrops_ = 0;
};

}  // namespace lamp
