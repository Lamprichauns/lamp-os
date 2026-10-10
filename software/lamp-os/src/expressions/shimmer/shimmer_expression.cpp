#include "expressions/shimmer/shimmer_expression.hpp"

#include <Arduino.h>
#include <algorithm>

#include "expressions/primitives.hpp"
#include "util/fade.hpp"

namespace lamp {

namespace {
constexpr ExpressionDescriptor kShimmerDescriptor =
    withMake(kShimmerDescriptorData, &makeExpr<ShimmerExpression>);
constexpr uint32_t kWindStepFactor = 3;
}  // namespace

const ExpressionDescriptor& ShimmerExpression::classDescriptor() {
  return kShimmerDescriptor;
}

const ExpressionDescriptor& ShimmerExpression::descriptor() const {
  return kShimmerDescriptor;
}

ShimmerExpression::ShimmerExpression(FrameBuffer* inBuffer, uint32_t inFrames)
    : Expression(inBuffer, inFrames) {}

void ShimmerExpression::configureFromParameters(
    const ExpressionParams& parameters) {
  const uint16_t window = windowSize();
  zone_ = resolveZone(parameters, window);

  uint32_t fire = getParam(parameters, "fire");
  if (fire > 3) fire = 3;
  style_ = fireStyle(fire);

  configureOpacity(parameters);

  const uint32_t count = getParam(parameters, "count", 0);
  const uint32_t burstMs = getParam(parameters, "durationMax", 0);
  const bool burst = count > 0 && burstMs > 0;
  burstCount_ = burst ? static_cast<uint16_t>(std::min<uint32_t>(count, zone_.size())) : 0;
  burstMs_ = burst ? clampBurstMs(burstMs) : 0;
}

void ShimmerExpression::onTrigger() {
  frame = 0;
  frames = kContinuousMaxFrames;
  lastUpdateMs_ = 0;
  windOffset_ = 0.0f;
  windTarget_ = 0.0f;
  flutter_ = 0.0f;
  nextGustMs_ = 0;

  cells_.clear();
  const uint16_t regionSize = zone_.size();
  if (regionSize == 0 || !fb || fb->pixelCount == 0) return;

  cells_.resize(regionSize);
  if (burstMs_ > 0) {
    burstStartMs_ = millis();
    // Selection sampling: exactly burstCount_ distinct cells spark, so a
    // full-count burst lights every one.
    uint16_t remaining = burstCount_;
    for (uint16_t i = 0; i < regionSize; ++i) {
      Cell& c = cells_[i];
      c.heat = style_.restLevel;
      const bool spark = remaining > 0 && rng.range(0, regionSize - i - 1) < remaining;
      c.target = spark ? rollSparkTarget(style_, rng) : style_.restLevel;
      if (spark) --remaining;
    }
    return;
  }
  for (Cell& c : cells_) {
    c.heat = style_.restLevel;
    c.target = rollHeatTarget(style_, rng);
  }
}

void ShimmerExpression::advanceWind(uint32_t nowMs, uint32_t deltaMs) {
  if (style_.windAmp <= 0.0f) {
    windOffset_ = 0.0f;
    return;
  }
  if (nextGustMs_ == 0 || timeReached(nowMs, nextGustMs_)) {
    windTarget_ = rollWindTarget(style_, rng);
    nextGustMs_ = nextGustAt(nowMs, style_, rng);
  }
  windOffset_ = approachHeat(windOffset_, windTarget_, deltaMs,
                             style_.stepMs * kWindStepFactor);
}

void ShimmerExpression::control() {
  continuousControl();
  // A burst concludes via frames = frame + 1; the completion dispatch lets
  // gcTransients() reap it.
  if (animationState == STOPPED && currentLoop > lastCompletedLoop) {
    onComplete();
    lastCompletedLoop = currentLoop;
  }
}

void ShimmerExpression::drawBurst() {
  const uint32_t nowMs = millis();
  const uint32_t deltaMs = (lastUpdateMs_ == 0) ? 0 : nowMs - lastUpdateMs_;
  lastUpdateMs_ = nowMs;
  const uint32_t elapsedMs = nowMs - burstStartMs_;

  const bool falling = burstFalling(elapsedMs, burstMs_);
  const uint32_t stepMs = burstStepMs(burstMs_);
  for (Cell& c : cells_) {
    if (falling) c.target = style_.restLevel;
    c.heat = approachHeat(c.heat, c.target, deltaMs, stepMs);
  }

  if (shouldAffectBuffer() && fb && fb->pixelCount > 0 && !cells_.empty()) {
    const float wispWeight = wispDimScale();
    const int end = std::min({static_cast<int>(zone_.posMax) + 1,
                              static_cast<int>(fb->pixelCount),
                              static_cast<int>(zone_.posMin) +
                                  static_cast<int>(cells_.size())});
    for (int i = static_cast<int>(zone_.posMin); i < end; ++i) {
      const float heat = clampUnit(cells_[static_cast<uint16_t>(i - zone_.posMin)].heat);
      if (heat < kBurstPaintHeat) continue;
      const Color anchor = fb->buffer[i];
      fb->buffer[i] = mixColorWeight(anchor, shimmerPaint(anchor, heat, style_, 1.0f), wispWeight);
    }
  }

  if (burstDone(elapsedMs, burstMs_)) {
    frames = frame + 1;
  } else {
    frame = rewindBeforeExhaust(frame, frames);
  }
  nextFrame();
}

void ShimmerExpression::draw() {
  if (burstMs_ > 0) {
    drawBurst();
    return;
  }
  const uint32_t nowMs = millis();
  const uint32_t deltaMs = (lastUpdateMs_ == 0) ? 0 : nowMs - lastUpdateMs_;
  lastUpdateMs_ = nowMs;

  const bool paint = shouldAffectBuffer() && zone_.size() > 0 && fb &&
                     fb->pixelCount > 0 && !cells_.empty();

  advanceWind(nowMs, deltaMs);
  flutter_ = advanceFlutter(flutter_, style_.flutterAmp, rng);
  for (Cell& c : cells_) {
    c.heat = approachHeat(c.heat, c.target, deltaMs, style_.stepMs);
    if (std::abs(c.target - c.heat) < 0.02f) {
      c.target = rollHeatTarget(style_, rng);
    }
  }

  if (paint) {
    const float wispWeight = wispDimScale();
    const float flutterScale = 1.0f + flutter_;
    // cells_ is sized on trigger, zone_ on configure; bound the index so a
    // future in-place reconfigure that grows the zone can't read past cells_.
    const int end = std::min({static_cast<int>(zone_.posMax) + 1,
                              static_cast<int>(fb->pixelCount),
                              static_cast<int>(zone_.posMin) +
                                  static_cast<int>(cells_.size())});
    for (int i = static_cast<int>(zone_.posMin); i < end; ++i) {
      const Cell& c = cells_[static_cast<uint16_t>(i - zone_.posMin)];
      const float heatR = clampUnit(c.heat + windOffset_);
      const Color anchor = fb->buffer[i];
      fb->buffer[i] = mixColorWeight(anchor, shimmerPaint(anchor, heatR, style_, flutterScale), wispWeight);
    }
  }

  frame = rewindBeforeExhaust(frame, frames);
  nextFrame();
}

}  // namespace lamp
