#pragma once

#include <algorithm>
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
// otherwise, so a retarget mid-ramp glides instead of snapping. A retarget
// while a home ramp still runs keeps at least that ramp's remaining time, so
// a crowd commit just after leaving home does not cut the second short.
class CrowdDim {
 public:
  float at(float engineFactor, bool ignoresPeers, uint32_t nowMs) {
    const float target = ignoresPeers ? 1.0f : engineFactor;
    const bool flipped = ignoresPeers != ignoring_;
    ignoring_ = ignoresPeers;
    settle(nowMs);
    if (target != target_) {
      uint32_t fadeMs = kCrowdDimFadeMs;
      if (flipped) {
        fadeMs = kCrowdDimHomeFadeMs;
      } else if (homeRamp_) {
        fadeMs = std::max(kCrowdDimFadeMs, fadeMs_ - elapsedMs(nowMs));
      }
      homeRamp_ = fadeMs > kCrowdDimFadeMs;
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
    if (elapsedMs(nowMs) >= fadeMs_) ramping_ = false;
    return true;
  }

 private:
  // Pins the finished ramp so a clock wrap cannot reopen it.
  void settle(uint32_t nowMs) {
    if (elapsedMs(nowMs) < fadeMs_) return;
    applied_ = target_;
    homeRamp_ = false;
  }

  // A caller may pass a timestamp taken before the ramp started (the loop's
  // `now` against a ramp begun inside the brightness tick); that reads as 0.
  uint32_t elapsedMs(uint32_t nowMs) const {
    const int32_t elapsed = static_cast<int32_t>(nowMs - startMs_);
    return elapsed < 0 ? 0 : static_cast<uint32_t>(elapsed);
  }

  float value(uint32_t nowMs) const {
    if (applied_ == target_) return target_;
    const uint32_t elapsed = elapsedMs(nowMs);
    if (elapsed >= fadeMs_) return target_;
    return applied_ + (target_ - applied_) * (static_cast<float>(elapsed) / fadeMs_);
  }

  float applied_ = 1.0f;
  float target_ = 1.0f;
  uint32_t fadeMs_ = 0;
  uint32_t startMs_ = 0;
  bool ignoring_ = false;
  bool ramping_ = false;
  // The ramp under way began as a home flip and has not finished.
  bool homeRamp_ = false;
};

}  // namespace lamp
