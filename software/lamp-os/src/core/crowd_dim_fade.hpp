#pragma once

#include <cstdint>

namespace lamp {

// Ramp for an ordinary crowd-dim deadband commit.
inline constexpr uint32_t kCrowdDimFadeMs = 80;
// Ramp when home mode starts or stops ignoring peers: the factor moves a long
// way at once, so it eases over a second.
inline constexpr uint32_t kCrowdDimHomeFadeMs = 1000;

// The crowd-dim factor brightness applies: the engine's factor, or 1.0 while
// home mode ignores peers. A new target ramps linearly from the current value,
// over kCrowdDimHomeFadeMs when the ignore state flipped and kCrowdDimFadeMs
// otherwise, so a retarget mid-ramp glides instead of snapping.
class CrowdDim {
 public:
  float at(float engineFactor, bool ignoresPeers, uint32_t nowMs) {
    const float target = ignoresPeers ? 1.0f : engineFactor;
    const uint32_t fadeMs = ignoresPeers != ignoring_ ? kCrowdDimHomeFadeMs : kCrowdDimFadeMs;
    ignoring_ = ignoresPeers;
    settle(nowMs);
    if (target != target_) {
      applied_ = value(nowMs);
      target_ = target;
      fadeMs_ = fadeMs;
      startMs_ = nowMs;
      ramping_ = true;
    }
    return value(nowMs);
  }

  // True on every pass while a ramp moves, and on the one pass after it ends
  // so the caller applies the landed value.
  bool fading(uint32_t nowMs) {
    if (!ramping_) return false;
    if (nowMs - startMs_ >= fadeMs_) ramping_ = false;
    return true;
  }

 private:
  // Pins the finished ramp so a clock wrap cannot reopen it.
  void settle(uint32_t nowMs) {
    if (applied_ != target_ && nowMs - startMs_ >= fadeMs_) applied_ = target_;
  }

  float value(uint32_t nowMs) const {
    if (applied_ == target_) return target_;
    const uint32_t elapsed = nowMs - startMs_;
    if (elapsed >= fadeMs_) return target_;
    return applied_ + (target_ - applied_) * (static_cast<float>(elapsed) / fadeMs_);
  }

  float applied_ = 1.0f;
  float target_ = 1.0f;
  uint32_t fadeMs_ = 0;
  uint32_t startMs_ = 0;
  bool ignoring_ = false;
  bool ramping_ = false;
};

}  // namespace lamp
