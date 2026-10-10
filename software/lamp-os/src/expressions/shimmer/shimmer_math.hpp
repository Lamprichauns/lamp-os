#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "util/color.hpp"

namespace lamp {

inline float clampUnit(float v) {
  return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// restLevel/amp are the heat band; stepMs the per-pixel flicker responsiveness
// (heat eases toward its target with this time constant, wind with 3x it);
// windAmp/gust* the global smooth gust (windAmp 0 = still). sparkChance is the
// per-roll probability a heat target instead lands in the hot [sparkLo,sparkHi]
// ember band (0 = never). flutterAmp is the amplitude of a global brightness
// random-walk layered on top of the smooth movement (0 = none), taking one step
// every flutterStepMs of wall time.
// warmthSwing/whiteHot shape how heat modulates the lamp's own colour (see
// warmthModulate): warmthSwing is the hue slide toward yellow at peak / maroon
// at floor (0 = pure brightness), whiteHot the desaturation toward white at the
// hot tip (0 = stays saturated). minBright is the brightness floor at zero heat;
// a low value lets a style go near-dark between embers. Starting calibration,
// tuned on the bench.
struct FireStyle {
  float restLevel;
  float amp;
  uint32_t stepMs;
  float windAmp;
  uint32_t gustLoMs;
  uint32_t gustHiMs;
  float sparkChance;
  float sparkLo;
  float sparkHi;
  float flutterAmp;
  uint32_t flutterStepMs;
  float warmthSwing;
  float whiteHot;
  float minBright;
};

inline FireStyle fireStyle(uint32_t value) {
  static constexpr FireStyle table[] = {
    // Twinkle: near-dark base, sparse sparks that rise and fade slowly, no wind.
    {0.00f, 0.02f, 900, 0.00f,    0,    0, 0.030f, 0.60f, 1.00f, 0.00f,  0, 0.15f, 0.00f, 0.80f},
    // Coals: a dim ember bed, several embers breathing in and out in place, no wind.
    {0.06f, 0.05f, 1600, 0.00f,    0,    0, 0.045f, 0.55f, 0.85f, 0.00f,  0, 0.30f, 0.00f, 0.40f},
    // Candle: a slow sway with a gentle flutter and the occasional soft
    // flicker-up. Bench dials: stepMs + gust* set the sway, flutter* the
    // shimmer on top, sparkChance how often a flicker-up comes.
    {
      .restLevel     = 0.36f,
      .amp           = 0.16f,
      .stepMs        = 500,    // per-pixel heat easing (ms); higher = slower sway
      .windAmp       = 0.08f,  // depth of the whole-flame sway
      .gustLoMs      = 2000,   // a new sway target every gustLo..gustHi ms
      .gustHiMs      = 4000,
      .sparkChance   = 0.020f, // per-retarget odds of a flicker-up
      .sparkLo       = 0.70f,  // flicker-up heat band
      .sparkHi       = 0.95f,
      .flutterAmp    = 0.040f, // flutter depth (fraction of brightness)
      .flutterStepMs = 80,     // flutter step period (ms); higher = calmer
      .warmthSwing   = 0.55f,
      .whiteHot      = 0.15f,
      .minBright     = 0.50f,
    },
    // Campfire: the liveliest and brightest, regular long gusts, a light
    // flutter and embers that flare up. Same dials as Candle.
    {
      .restLevel     = 0.50f,
      .amp           = 0.28f,
      .stepMs        = 300,    // per-pixel heat easing (ms); higher = slower
      .windAmp       = 0.18f,  // depth of the gusts
      .gustLoMs      = 3500,   // a new gust target every gustLo..gustHi ms
      .gustHiMs      = 7000,
      .sparkChance   = 0.012f, // per-retarget odds of an ember flare
      .sparkLo       = 0.75f,  // flare heat band
      .sparkHi       = 0.95f,
      .flutterAmp    = 0.015f, // flutter depth (fraction of brightness)
      .flutterStepMs = 40,     // flutter step period (ms); higher = calmer
      .warmthSwing   = 0.80f,
      .whiteHot      = 0.40f,
      .minBright     = 0.30f,
    },
  };
  if (value > 3) value = 3;
  return table[value];
}

template <class Rng>
inline float rollHeatTarget(const FireStyle& s, Rng& rng) {
  if (s.sparkChance > 0.0f &&
      rng.range(0, 1000) / 1000.0f < s.sparkChance) {
    const float t = rng.range(0, 1000) / 1000.0f;
    return clampUnit(s.sparkLo + (s.sparkHi - s.sparkLo) * t);
  }
  const float lo = clampUnit(s.restLevel - s.amp);
  const float hi = clampUnit(s.restLevel + s.amp);
  const float t = rng.range(0, 1000) / 1000.0f;
  return lo + (hi - lo) * t;
}

// Global gust target in [-windAmp, +windAmp]. windAmp 0 (still air) yields 0.
template <class Rng>
inline float rollWindTarget(const FireStyle& s, Rng& rng) {
  return (rng.range(0, 2000) / 1000.0f - 1.0f) * s.windAmp;
}

template <class Rng>
inline uint32_t nextGustAt(uint32_t nowMs, const FireStyle& s, Rng& rng) {
  return nowMs + rng.range(s.gustLoMs, s.gustHiMs);
}

inline constexpr float kFlutterStepFrac = 0.5f;
// Wall time a single draw may catch up on. A longer gap (a stall) drops the
// excess so the walk resumes rather than replaying seconds of steps at once.
inline constexpr uint32_t kFlutterMaxCatchUpMs = 250;

// A bounded random-walk in [-amp, +amp] stepped on wall time. from/to are the
// walk's last two points and carryMs the time since `to` was rolled; the shown
// value glides from one to the other across each step.
struct Flutter {
  float from = 0.0f;
  float to = 0.0f;
  uint32_t carryMs = 0;
};

// Advance the walk by deltaMs and return the value to show. One step every
// stepMs nudges by up to +/- kFlutterStepFrac*amp and clamps, so the motion
// depends only on elapsed time, never on how many draws covered it. amp 0
// yields 0 and draws no rng.
template <class Rng>
inline float advanceFlutter(Flutter& f, float amp, uint32_t stepMs,
                            uint32_t deltaMs, Rng& rng) {
  if (amp <= 0.0f || stepMs == 0) return 0.0f;
  f.carryMs += std::min(deltaMs, kFlutterMaxCatchUpMs);
  const float step = amp * kFlutterStepFrac;
  while (f.carryMs >= stepMs) {
    f.carryMs -= stepMs;
    f.from = f.to;
    const float v = f.to + (rng.range(0, 2000) / 1000.0f - 1.0f) * step;
    f.to = v < -amp ? -amp : (v > amp ? amp : v);
  }
  return f.from + (f.to - f.from) * (static_cast<float>(f.carryMs) / stepMs);
}

// Fraction of the remaining distance to close over deltaMs with time constant
// stepMs. Exponential, so any split of the same wall time lands in the same
// place and a long gap settles on the target without overshoot.
inline float approachFactor(uint32_t deltaMs, uint32_t stepMs) {
  if (stepMs == 0) return 1.0f;
  return 1.0f - std::exp(-static_cast<float>(deltaMs) / stepMs);
}

inline float approachHeat(float heat, float target, float factor) {
  return heat + (target - heat) * factor;
}

inline float heatBrightness(float heat, float minBright) {
  return minBright + (1.0f - minBright) * clampUnit(heat);
}

inline constexpr float kShimmerColdFloor = 0.20f;   // channel scale at coldest
inline constexpr float kShimmerHotGain = 0.35f;     // brightness lift at hottest
inline constexpr float kShimmerColdBlueKill = 1.3f; // blue attenuates past green when cold
inline constexpr uint32_t kShimmerFloorPct = 8;     // faded cells settle to this % of their own colour, not black

// Modulate the anchor colour along a dim->warm ramp driven by heat. neutral is
// the style rest heat: heat == neutral returns the anchor unchanged. Above it
// the anchor brightens and (warmthSwing) slides green toward red for a yellower
// tip, with whiteHot engaging W at the very top; below it the anchor darkens and
// green/blue fall faster than red for a maroon floor. Channel-space only, no HSV
// round-trip; a black anchor stays black.
inline Color warmthModulate(Color anchor, float heat, float neutral,
                            float warmthSwing, float whiteHot) {
  float w;
  if (heat >= neutral)
    w = (neutral < 1.0f) ? (heat - neutral) / (1.0f - neutral) : 0.0f;
  else
    w = (neutral > 0.0f) ? (heat - neutral) / neutral : 0.0f;
  w = w < -1.0f ? -1.0f : (w > 1.0f ? 1.0f : w);

  const float bscale = (w >= 0.0f) ? 1.0f + w * kShimmerHotGain
                                   : 1.0f + w * (1.0f - kShimmerColdFloor);
  float r = anchor.r * bscale;
  float g = anchor.g * bscale;
  float b = anchor.b * bscale;
  float wc = anchor.w * bscale;

  if (warmthSwing > 0.0f) {
    const float hot = warmthSwing * (w > 0.0f ? w : 0.0f);
    const float cold = warmthSwing * (w < 0.0f ? -w : 0.0f);
    g += (r - g) * hot;
    g *= 1.0f - cold;
    b *= 1.0f - cold * kShimmerColdBlueKill;
  }

  if (whiteHot > 0.0f && w > 0.0f) {
    const float lum = std::max(r, std::max(g, b)) / 255.0f;
    wc += (255.0f - wc) * whiteHot * w * clampUnit(lum);
  }

  auto clamp8 = [](float v) -> uint8_t {
    return v <= 0.0f ? 0 : (v >= 255.0f ? 255 : static_cast<uint8_t>(v + 0.5f));
  };
  return Color{clamp8(r), clamp8(g), clamp8(b), clamp8(wc)};
}

}  // namespace lamp
