#include "./pulse_expression.hpp"

#include <Arduino.h>
#include <algorithm>
#include <cmath>

namespace lamp {

// Use a large frame count to let wave position determine animation end
// This prevents premature stopping based on frame count
static constexpr uint32_t PULSE_MAX_FRAMES = 10000;

PulseExpression::PulseExpression(FrameBuffer* inBuffer, uint32_t inFrames)
    : Expression(inBuffer, inFrames) {
  isExclusive = false;  // This can run and blend with other things
  allowedInHomeMode = true; 
}

void PulseExpression::configureFromParameters(const std::map<std::string, std::variant<uint32_t, float, double>>& parameters) {
  // Extract pulse speed parameter with default value
  uint32_t pulseSpeed = extractUint32Parameter(parameters, "pulseSpeed", 3);

  // Pulse-specific configuration
  // pulseSpeed is total travel time in seconds (1-10s)
  // Convert to ms per pixel: total_time_ms / pixel_count
  if (fb && fb->pixelCount > 0) {
    pulseSpeedMs = (pulseSpeed * 1000) / fb->pixelCount;
    pulseSpeedMs = std::max((uint32_t)10, pulseSpeedMs);  // Minimum 10ms per pixel
  } else {
    pulseSpeedMs = 100;  // Default fallback
  }

  // Pulse width constant
  static constexpr uint32_t PULSE_WIDTH = 15;
  pulseWidth = PULSE_WIDTH;

  // Default to white if no colors provided
  if (colors.empty()) {
    colors.push_back(Color(255, 255, 255, 255));
  }

  // Start with first color
  pulseColor = colors[0];
}

float PulseExpression::calculateBlendFactor(int pixelIndex) const {
  float distance = std::abs(static_cast<float>(pixelIndex) - wavePosition);

  // No blend if outside pulse width
  if (distance > pulseWidth) {
    return 0.0f;
  }

  // Special case: if very close to center (within 0.5), give full strength
  if (distance < 0.5f) {
    return 1.0f;
  }

  // Simplified quadratic falloff for performance
  // Avoids expensive exp() calculation
  float normalizedDist = distance / static_cast<float>(pulseWidth);
  float factor = 1.0f - (normalizedDist * normalizedDist);  // Quadratic falloff
  factor = std::max(0.0f, factor);  // Clamp to 0

  return factor;
}

void PulseExpression::updateWavePosition() {
  uint32_t currentMs = millis();

  if (lastUpdateMs == 0) {
    lastUpdateMs = currentMs;
    return;
  }

  uint32_t deltaMs = currentMs - lastUpdateMs;

  // Calculate how far to move based on speed
  float pixelsToMove = static_cast<float>(deltaMs) / static_cast<float>(pulseSpeedMs);

  wavePosition += pixelsToMove * waveDirection;

  // Continue moving wave past the end of the strip
  // Animation will stop when frame >= frames via nextFrame()
  // Wave should reach position (pixelCount + pulseWidth) before stopping

  lastUpdateMs = currentMs;
}

void PulseExpression::selectNextColor() {
  if (colors.size() > 1) {
    // Pick random color from palette
    pulseColor = getRandomColor();
  }
}

void PulseExpression::onTrigger() {
#ifdef LAMP_DEBUG
  Serial.printf("PulseExpression::onTrigger() - starting pulse animation\n");
#endif
  // Reset wave to start position
  wavePosition = -static_cast<float>(pulseWidth);  // Start just off the strip
  waveDirection = 1;  // Always move forward
  lastUpdateMs = 0;
  selectNextColor();

  // Calculate frames needed for full animation
  // Wave needs to travel from -pulseWidth to pixelCount + pulseWidth
  float totalDistance = fb->pixelCount + (2.0f * pulseWidth);
  float timeNeededMs = totalDistance * pulseSpeedMs;

  // Set frames high enough that wave position will determine when to stop
  // The animation will complete when wave reaches pixelCount + (2 * pulseWidth)
  frames = PULSE_MAX_FRAMES;
  frame = 0;

#ifdef LAMP_DEBUG
  Serial.printf("PulseExpression: Start at %.1f, end at %lu, totalDistance=%.1f, timeNeededMs=%.1f, frames=%lu\n",
                wavePosition, fb->pixelCount + pulseWidth, totalDistance, timeNeededMs, frames);
#endif
}

void PulseExpression::onUpdate() {
  // Always update wave position to ensure smooth fade-out
  updateWavePosition();
}

void PulseExpression::draw() {
#ifdef LAMP_DEBUG
  static uint32_t lastDrawDebugMs = 0;
  if (millis() - lastDrawDebugMs > 500) {  // Log twice per second for better visibility
    Serial.printf("Pulse draw - wavePosition: %.2f (range: -%lu to %lu), frame: %lu/%lu, animState: %d\n",
                  wavePosition, pulseWidth, fb->pixelCount + pulseWidth, frame, frames, animationState);
    lastDrawDebugMs = millis();
  }
#endif

  // Pause if an exclusive behavior is running
  if (shouldPause()) return;

  // For pulse, we need to continue drawing even when "stopped" to allow fade-out
  // Only skip if we shouldn't affect this buffer based on target
  if (!shouldAffectBuffer() && animationState != STOPPED) {
#ifdef LAMP_DEBUG
    Serial.printf("PulseExpression::draw() - skipping: shouldAffectBuffer=%d\n", shouldAffectBuffer());
#endif
    return;
  }

  // Continue updating wave position even when STOPPED to complete fade-out
  if (animationState == STOPPED && wavePosition <= fb->pixelCount + (2 * pulseWidth)) {
    updateWavePosition();
  }

  // Apply pulse effect
  int pixelsAffected = 0;
  for (int i = 0; i < fb->pixelCount; i++) {
    float blendFactor = calculateBlendFactor(i);

    if (blendFactor > 0.001f) {  // Skip pixels with negligible blend
      // Blend pulse color with current buffer
      fb->buffer[i] = fb->buffer[i].lerp(pulseColor, blendFactor);
      pixelsAffected++;

    }
  }


  // Advance animation frame
  nextFrame();

  // Check if wave has completely cleared the strip
  // Wave extends pulseWidth on both sides, so center needs to be at pixelCount + (2 * pulseWidth)
  // for the trailing edge to clear pixelCount
  if (wavePosition > fb->pixelCount + (2 * pulseWidth)) {
#ifdef LAMP_DEBUG
    Serial.printf("PulseExpression::draw() - wave cleared strip at position %.2f\n", wavePosition);
#endif
    // Wave has completely passed, safe to stop
    if (animationState != STOPPED) {
      stop();
    }
  }

  // Check if animation just completed
  if (animationState == STOPPED && frame == 0) {  // frame resets to 0 when stopped
#ifdef LAMP_DEBUG
    Serial.printf("PulseExpression::draw() - animation completed, final wavePosition: %.2f (target: %lu)\n",
                  wavePosition, fb->pixelCount + pulseWidth);
#endif
  }
}

}  // namespace lamp