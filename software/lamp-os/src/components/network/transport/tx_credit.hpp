#pragma once

#include <atomic>
#include <cstdint>

namespace lamp {

// Bounds the ESP-NOW frames handed to the WiFi driver and not yet reported
// sent. Each queued frame holds a driver TX buffer from the heap until it
// airs; past the driver's queue a send fails with ESP_ERR_ESPNOW_NO_MEM, and
// a burst of those preceded the TX path wedging with its buffers held. A
// send takes a credit before esp_now_send and the send callback returns one.
// IDF documents no guarantee of one callback per accepted send (esp_now.h
// says nothing of it, and the driver is closed source), so each credit
// carries the time it was taken, and one out for kStallMs is presumed lost
// and taken back, whether or not other frames are completing. Lock-free: the
// loop and WiFi tasks both send, the WiFi task completes.
class TxCredit {
 public:
  static constexpr uint16_t kMaxInFlight = 6;
  static constexpr uint32_t kStallMs = 2000;

  // True, with a credit taken, when a frame may be submitted.
  bool tryAcquire(uint32_t nowMs) {
    reclaimStale(nowMs);
    const uint32_t stamp = nowMs | 1u;  // 0 marks a free slot
    for (auto& slot : slots_) {
      uint32_t expected = 0;
      if (slot.compare_exchange_strong(expected, stamp, std::memory_order_relaxed)) return true;
    }
    refused_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  // A submit the driver refused returns its credit at once.
  void cancel() { releaseNewest(); }
  // The send callback: one frame left the queue. Callbacks name no frame, so
  // the newest credit goes: credits are alike, and the oldest left out is the
  // one a lost callback would have returned, so it ages into kStallMs while
  // later frames keep completing.
  void complete(uint32_t /*nowMs*/) { releaseNewest(); }

  uint16_t inFlight() const {
    uint16_t n = 0;
    for (const auto& slot : slots_) n += slot.load(std::memory_order_relaxed) != 0;
    return n;
  }
  // True when a frame may be submitted now, with stale credits taken back
  // first, so a caller that waits for a credit before submitting is never
  // held by credits lost on a quiet link.
  bool available(uint32_t nowMs) {
    reclaimStale(nowMs);
    return inFlight() < kMaxInFlight;
  }
  // Credits presumed lost and taken back.
  uint32_t stalls() const { return stalls_.load(std::memory_order_relaxed); }
  // Sends refused for want of a credit: a mesh resend or any other send that
  // does not wait on a free credit first, and is dropped.
  uint32_t refused() const { return refused_.load(std::memory_order_relaxed); }

 private:
  static int32_t elapsedSince(uint32_t thenMs, uint32_t nowMs) {
    return static_cast<int32_t>(nowMs - thenMs);
  }
  // A stamp taken just after `nowMs` was read (the other task) reads as not
  // yet stale.
  void reclaimStale(uint32_t nowMs) {
    for (auto& slot : slots_) {
      uint32_t stamp = slot.load(std::memory_order_relaxed);
      if (stamp != 0 && elapsedSince(stamp & ~1u, nowMs) >= static_cast<int32_t>(kStallMs) &&
          slot.compare_exchange_strong(stamp, 0, std::memory_order_relaxed)) {
        stalls_.fetch_add(1, std::memory_order_relaxed);
      }
    }
  }
  void releaseNewest() {
    while (true) {
      std::atomic<uint32_t>* pick = nullptr;
      uint32_t pickStamp = 0;
      for (auto& slot : slots_) {
        const uint32_t stamp = slot.load(std::memory_order_relaxed);
        if (stamp != 0 && (pick == nullptr || elapsedSince(pickStamp, stamp) > 0)) {
          pick = &slot;
          pickStamp = stamp;
        }
      }
      if (pick == nullptr) return;
      if (pick->compare_exchange_strong(pickStamp, 0, std::memory_order_relaxed)) return;
    }
  }
  std::atomic<uint32_t> slots_[kMaxInFlight] = {};
  std::atomic<uint32_t> stalls_{0};
  std::atomic<uint32_t> refused_{0};
};

// One submit under a credit: `send()` hands the frame to the driver and
// returns 0 on success or its error; a refused submit returns the credit.
// `noCredit` is the error reported when no credit is free.
template <class Send>
int submitWithCredit(TxCredit& credit, uint32_t nowMs, int noCredit, Send&& send) {
  if (!credit.tryAcquire(nowMs)) return noCredit;
  const int err = send();
  if (err != 0) credit.cancel();
  return err;
}

}  // namespace lamp
