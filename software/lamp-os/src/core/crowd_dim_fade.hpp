#pragma once

#include <cstdint>

#include "core/home_mode_gate.hpp"

namespace lamp {

// The crowd-dim factor to aim for: none while home mode ignores other lamps.
constexpr float crowdDimTarget(float engineFactor, bool homeActive, bool socialDisabled) {
  return homeModeBlocksRemote(homeActive, socialDisabled, TransientSource::Remote)
             ? 1.0f
             : engineFactor;
}

// Crowd-dim factor easing toward a moving target. A target that differs from
// the last one restarts a linear ramp from the current value over the fadeMs
// passed with it, so a retarget mid-fade glides instead of snapping.
class CrowdDimFade {
 public:
  float at(float target, uint32_t fadeMs, uint32_t nowMs) {
    if (target != target_) {
      applied_ = value(nowMs);
      target_ = target;
      fadeMs_ = fadeMs;
      startMs_ = nowMs;
    }
    return value(nowMs);
  }

  bool fading(uint32_t nowMs) const { return applied_ != target_ && nowMs - startMs_ < fadeMs_; }

 private:
  float value(uint32_t nowMs) const {
    const uint32_t elapsed = nowMs - startMs_;
    if (elapsed >= fadeMs_) return target_;
    return applied_ + (target_ - applied_) * (static_cast<float>(elapsed) / fadeMs_);
  }

  float applied_ = 1.0f;
  float target_ = 1.0f;
  uint32_t fadeMs_ = 0;
  uint32_t startMs_ = 0;
};

}  // namespace lamp
