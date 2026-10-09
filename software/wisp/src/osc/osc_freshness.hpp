// Liveness of the OSC palette stream. A palette marks it live; kStaleMs
// without one marks it stale. The controller holds paint at Off while stale
// and keeps paint mode, so the next palette resumes painting on its own.
// Loop task only.

#pragma once

#include <cstddef>
#include <cstdint>

namespace wisp {

class OscFreshness {
 public:
  // Straylight's UDP feed is change-only with a full restate every 10 s, so a
  // still palette arrives only on the restate. Too close to 10 s and lamps
  // pulse home on every restate cycle.
  static constexpr uint32_t kStaleMs = 30000;

  // Records a palette arrival. True on the stale -> live edge.
  bool onPalette(uint32_t nowMs) {
    lastRxMs_ = nowMs;
    const bool edge = !live_;
    live_ = true;
    return edge;
  }

  // True on the live -> stale edge.
  bool tick(uint32_t nowMs) {
    if (!live_ || nowMs - lastRxMs_ < kStaleMs) return false;
    live_ = false;
    return true;
  }

  void reset() { live_ = false; }
  bool live() const { return live_; }

 private:
  bool live_ = false;
  uint32_t lastRxMs_ = 0;
};

// True when no channel is above zero (NaN included): the palette would paint
// lamps black. Straylight sends zeros for a role with nothing bound to it.
inline bool isBlankPalette(const float* rgb, size_t floats) {
  for (size_t i = 0; i < floats; ++i) {
    if (rgb[i] > 0.0f) return false;
  }
  return true;
}

}  // namespace wisp
