#include "./glitchy_expression.hpp"

namespace lamp {

GlitchyExpression::GlitchyExpression(FrameBuffer* inBuffer, uint32_t inFrames)
    : Expression(inBuffer, inFrames) {
  allowedInHomeMode = false;  // Glitchy should not work in home mode
  isExclusive = true;  // Glitchy takes exclusive control when active
}

void GlitchyExpression::configureFromParameters(const std::map<std::string, std::variant<uint32_t, float, double>>& parameters) {
  // Extract duration range parameters with default values
  uint32_t durationMin = extractUint32Parameter(parameters, "durationMin", 1);
  uint32_t durationMax = extractUint32Parameter(parameters, "durationMax", 3);

  // Validate and set duration range
  glitchDurationMin = durationMin > 0 && durationMin <= 60 ? durationMin : 1;
  glitchDurationMax = durationMax > 0 && durationMax <= 60 ? durationMax : 3;

  // Ensure max is at least as large as min
  if (glitchDurationMax < glitchDurationMin) {
    glitchDurationMax = glitchDurationMin;
  }
}

void GlitchyExpression::onTrigger() {
  glitchColor = getRandomColor();

  // Randomly pick duration between min and max
  if (glitchDurationMin == glitchDurationMax) {
    frames = glitchDurationMin;
  } else {
    std::uniform_int_distribution<uint32_t> durationDist(glitchDurationMin, glitchDurationMax);
    frames = durationDist(rng);
  }

#ifdef LAMP_DEBUG
  Serial.printf("GlitchyExpression::onTrigger() - starting glitch animation with color R%d G%d B%d W%d, duration: %lu frames (range %lu-%lu)\n",
                glitchColor.r, glitchColor.g, glitchColor.b, glitchColor.w, frames, glitchDurationMin, glitchDurationMax);
  Serial.printf("GlitchyExpression::onTrigger() - pixelCount: %d\n", fb->pixelCount);
#endif
}

void GlitchyExpression::draw() {
#ifdef LAMP_DEBUG
  static uint32_t lastDrawDebugMs = 0;
  if (millis() - lastDrawDebugMs > 1000) {  // Log once per second
    Serial.printf("Glitchy draw - frame: %lu/%lu, animState: %d, shouldAffect: %d, isLastFrame: %d\n",
                  frame, frames, animationState, shouldAffectBuffer(), isLastFrame());
    lastDrawDebugMs = millis();
  }
#endif

  if (!shouldAffectBuffer()) {
#ifdef LAMP_DEBUG
    Serial.printf("GlitchyExpression::draw() - skipping: shouldAffectBuffer returned false\n");
#endif
    nextFrame();
    return;
  }

  // On last frame, restore original buffer
  if (isLastFrame()) {
#ifdef LAMP_DEBUG
    Serial.printf("GlitchyExpression::draw() - last frame, restoring original buffer\n");
#endif
    fb->buffer = savedBuffer;
  } else {
    // Blend glitch color with current buffer for a tinted effect
    static constexpr float GLITCH_BLEND_FACTOR = 0.95f;
    for (int i = 0; i < fb->pixelCount; i++) {
      fb->buffer[i] = fb->buffer[i].lerp(glitchColor, GLITCH_BLEND_FACTOR);
    }
#ifdef LAMP_DEBUG
    static uint32_t lastGlitchDebugMs = 0;
    if (millis() - lastGlitchDebugMs > 500) {  // Log glitch application every 500ms
      Serial.printf("GlitchyExpression::draw() - applied glitch color to %d pixels (frame %lu/%lu)\n",
                    fb->pixelCount, frame, frames);
      lastGlitchDebugMs = millis();
    }
#endif
  }

  nextFrame();

  // Check if animation just completed
  if (animationState == STOPPED && frame >= frames) {
#ifdef LAMP_DEBUG
    Serial.printf("GlitchyExpression::draw() - glitch animation completed at frame %lu\n", frame);
#endif
  }
}

}  // namespace lamp