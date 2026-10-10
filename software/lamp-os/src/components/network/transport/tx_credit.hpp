#pragma once

#include <atomic>
#include <cstdint>

namespace lamp {

// Bounds the ESP-NOW frames handed to the WiFi driver and not yet reported
// sent. Each queued frame holds a driver TX buffer from the heap until it
// airs; past the driver's queue a send fails with ESP_ERR_ESPNOW_NO_MEM, and
// a burst of those preceded the TX path wedging with its buffers held. A
// send takes a credit before esp_now_send and the send callback returns it.
// If no credit moves for kStallMs while every credit is out, the count is
// presumed lost (a callback that never fires) and resets. Lock-free: the
// loop and WiFi tasks both send, the WiFi task completes.
class TxCredit {
 public:
  static constexpr uint16_t kMaxInFlight = 6;
  static constexpr uint32_t kStallMs = 2000;

  // True, with a credit taken, when a frame may be submitted.
  bool tryAcquire(uint32_t nowMs) {
    uint16_t cur = inFlight_.load(std::memory_order_relaxed);
    while (true) {
      if (cur >= kMaxInFlight) {
        if (nowMs - lastMoveMs_.load(std::memory_order_relaxed) < kStallMs) return false;
        stalls_.fetch_add(1, std::memory_order_relaxed);
        inFlight_.store(0, std::memory_order_relaxed);
        cur = 0;
      }
      if (inFlight_.compare_exchange_weak(cur, static_cast<uint16_t>(cur + 1),
                                          std::memory_order_relaxed)) {
        lastMoveMs_.store(nowMs, std::memory_order_relaxed);
        return true;
      }
    }
  }
  // A submit the driver refused returns its credit at once.
  void cancel() { release(); }
  // The send callback: one frame left the queue.
  void complete(uint32_t nowMs) {
    lastMoveMs_.store(nowMs, std::memory_order_relaxed);
    release();
  }

  uint16_t inFlight() const { return inFlight_.load(std::memory_order_relaxed); }
  uint32_t stalls() const { return stalls_.load(std::memory_order_relaxed); }

 private:
  void release() {
    uint16_t cur = inFlight_.load(std::memory_order_relaxed);
    while (cur > 0 && !inFlight_.compare_exchange_weak(
                          cur, static_cast<uint16_t>(cur - 1), std::memory_order_relaxed)) {
    }
  }
  std::atomic<uint16_t> inFlight_{0};
  std::atomic<uint32_t> lastMoveMs_{0};
  std::atomic<uint32_t> stalls_{0};
};

}  // namespace lamp
