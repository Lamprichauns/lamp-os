#pragma once

#include <atomic>
#include <cstdint>

namespace lamp {

// Bounds the ESP-NOW frames handed to the WiFi driver and not yet reported
// sent. Each queued frame holds a driver TX buffer from the heap until it
// airs; past the driver's queue a send fails with ESP_ERR_ESPNOW_NO_MEM, and
// a burst of those preceded the TX path wedging with its buffers held. A
// send takes a credit before esp_now_send and the send callback returns it.
// If credits are out and none has come back for kStallMs, the outstanding
// ones are presumed lost (callbacks that never fire) and the count resets.
// Lock-free: the loop and WiFi tasks both send, the WiFi task completes.
class TxCredit {
 public:
  static constexpr uint16_t kMaxInFlight = 6;
  static constexpr uint32_t kStallMs = 2000;

  // True, with a credit taken, when a frame may be submitted.
  bool tryAcquire(uint32_t nowMs) {
    uint16_t cur = inFlight_.load(std::memory_order_relaxed);
    if (cur > 0 && elapsedSince(lastProgressMs_.load(std::memory_order_relaxed), nowMs) >=
                       static_cast<int32_t>(kStallMs)) {
      stalls_.fetch_add(1, std::memory_order_relaxed);
      inFlight_.store(0, std::memory_order_relaxed);
      cur = 0;
    }
    while (true) {
      if (cur >= kMaxInFlight) return false;
      if (inFlight_.compare_exchange_weak(cur, static_cast<uint16_t>(cur + 1),
                                          std::memory_order_relaxed)) {
        // The stall clock starts with the first frame out, not at the last
        // completion, which may be long past.
        if (cur == 0) advance(nowMs, true);
        return true;
      }
    }
  }
  // A submit the driver refused returns its credit at once.
  void cancel() { release(); }
  // The send callback: one frame left the queue.
  void complete(uint32_t nowMs) {
    advance(nowMs, false);
    release();
  }

  uint16_t inFlight() const { return inFlight_.load(std::memory_order_relaxed); }
  // Times the count was presumed lost and reset.
  uint32_t stalls() const { return stalls_.load(std::memory_order_relaxed); }

 private:
  static int32_t elapsedSince(uint32_t thenMs, uint32_t nowMs) {
    return static_cast<int32_t>(nowMs - thenMs);
  }
  // The loop and WiFi tasks read millis() at different moments, so a store
  // never moves the clock backwards; `force` restarts it.
  void advance(uint32_t nowMs, bool force) {
    uint32_t cur = lastProgressMs_.load(std::memory_order_relaxed);
    while ((force || elapsedSince(cur, nowMs) > 0) &&
           !lastProgressMs_.compare_exchange_weak(cur, nowMs, std::memory_order_relaxed)) {
    }
  }
  void release() {
    uint16_t cur = inFlight_.load(std::memory_order_relaxed);
    while (cur > 0 && !inFlight_.compare_exchange_weak(
                          cur, static_cast<uint16_t>(cur - 1), std::memory_order_relaxed)) {
    }
  }
  std::atomic<uint16_t> inFlight_{0};
  std::atomic<uint32_t> lastProgressMs_{0};
  std::atomic<uint32_t> stalls_{0};
};

}  // namespace lamp
