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
    uint16_t cur = resetIfStalled(nowMs);
    while (true) {
      if (cur >= kMaxInFlight) {
        refused_.fetch_add(1, std::memory_order_relaxed);
        return false;
      }
      if (inFlight_.compare_exchange_weak(cur, static_cast<uint16_t>(cur + 1),
                                          std::memory_order_relaxed)) {
        // The stall clock restarts with the first frame out after an idle
        // spell.
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
  // True when a frame may be submitted now, with the stall reset applied
  // first, so a caller that waits for a credit before submitting is never
  // held by credits lost on a quiet link.
  bool available(uint32_t nowMs) { return resetIfStalled(nowMs) < kMaxInFlight; }
  // Times the count was presumed lost and reset.
  uint32_t stalls() const { return stalls_.load(std::memory_order_relaxed); }
  // Sends refused for want of a credit: a mesh resend or any other send that
  // does not wait on a free credit first, and is dropped.
  uint32_t refused() const { return refused_.load(std::memory_order_relaxed); }

 private:
  // The credits out, after presuming them lost when none has come back for
  // kStallMs.
  uint16_t resetIfStalled(uint32_t nowMs) {
    const uint16_t cur = inFlight_.load(std::memory_order_relaxed);
    if (cur > 0 && elapsedSince(lastProgressMs_.load(std::memory_order_relaxed), nowMs) >=
                       static_cast<int32_t>(kStallMs)) {
      stalls_.fetch_add(1, std::memory_order_relaxed);
      inFlight_.store(0, std::memory_order_relaxed);
      return 0;
    }
    return cur;
  }
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
