// Native pin for the Breathing swell against the real BreathingExpression: a
// transient carrying durationMax runs one swell of that length and completes
// so the transient GC reaps it, while a configured breathing carrying the same
// key keeps its user-set cycle.

#include <unity.h>

#include "../../src/core/animated_behavior.cpp"
#include "../../src/core/frame_buffer.cpp"
#include "../../src/expressions/breathing/breathing_expression.cpp"
#include "../../src/expressions/expression.cpp"
#include "../../src/util/color.cpp"
#include "../../src/util/fade.cpp"

using namespace lamp;

namespace lamp {
OverrideAggregate overrides;
void ExpressionManager::onExpressionFired(Expression*) {}
}  // namespace lamp

namespace {

constexpr uint8_t kPixels = 24;
constexpr uint32_t kSwellMs = 300;
constexpr uint32_t kFrameMs = 16;
const Color kAnchor(0, 0, 0, 0);
const Color kTint(0, 0, 255, 0);

struct Surfaces {
  FrameBuffer base;
  BehaviorContext ctx;
  Surfaces() {
    base.pixelCount = kPixels;
    base.buffer.assign(kPixels, kAnchor);
    ctx.expressionFrameBuffers = {&base, &base};
  }
};

void swellOn(BreathingExpression& e, Surfaces& s, bool transient, uint32_t swellMs = kSwellMs) {
  e.configure({kTint}, 60, 900, TARGET_BASE);
  e.configureFromParameters({{"breathSpeed", 8}, {"durationMax", swellMs}, {"opacity", 100}});
  e.isTransient = transient;
  e.autoTriggerEnabled = !transient;
  e.setBehaviorContext(&s.ctx);
}

// One compositor frame; returns the most blue any pixel took.
uint8_t frame(BreathingExpression& e, Surfaces& s, uint32_t nowMs) {
  set_mock_millis(nowMs);
  s.base.buffer.assign(kPixels, kAnchor);
  e.control();
  e.draw();
  uint8_t blue = 0;
  for (const Color& c : s.base.buffer) blue = c.b > blue ? c.b : blue;
  return blue;
}

// Frames until the instance completes; fails past `limitMs`.
uint32_t runToCompletion(BreathingExpression& e, Surfaces& s, uint32_t limitMs,
                         uint8_t* peak = nullptr) {
  set_mock_millis(1000);
  TEST_ASSERT_TRUE(e.trigger());
  uint32_t now = 1000;
  for (; !e.isAnimationComplete(); now += kFrameMs) {
    const uint8_t blue = frame(e, s, now);
    if (peak && blue > *peak) *peak = blue;
    TEST_ASSERT_TRUE(now - 1000 < limitMs);
  }
  return now - 1000;
}

}  // namespace

void setUp() { set_mock_millis(1000); }
void tearDown() {}

void test_a_transient_swell_completes_after_one_swell() {
  Surfaces s;
  BreathingExpression e(&s.base);
  swellOn(e, s, /*transient=*/true);
  uint8_t peak = 0;
  const uint32_t took = runToCompletion(e, s, kSwellMs + 4 * kFrameMs, &peak);
  TEST_ASSERT_TRUE(took >= kSwellMs);
  TEST_ASSERT_TRUE(peak > 200);
}

void test_a_swell_ends_dark() {
  Surfaces s;
  BreathingExpression e(&s.base);
  swellOn(e, s, /*transient=*/true);
  runToCompletion(e, s, kSwellMs + 4 * kFrameMs);
  TEST_ASSERT_EQUAL_UINT8(0, frame(e, s, 2000));
}

void test_a_swell_below_the_floor_clamps_to_it() {
  Surfaces s;
  BreathingExpression e(&s.base);
  swellOn(e, s, /*transient=*/true, 10);
  TEST_ASSERT_TRUE(runToCompletion(e, s, 100 + 4 * kFrameMs) >= 100);
}

void test_a_configured_breathing_with_the_key_keeps_its_cycle() {
  Surfaces s;
  BreathingExpression e(&s.base);
  swellOn(e, s, /*transient=*/false);
  e.trigger();
  for (uint32_t now = 1000; now < 1000 + 5 * kSwellMs; now += kFrameMs) frame(e, s, now);
  TEST_ASSERT_FALSE(e.isAnimationComplete());
  // 1.5 s into an 8 s cycle the breath is still on its way up.
  const uint8_t early = frame(e, s, 2500);
  TEST_ASSERT_TRUE(early < frame(e, s, 3500));
}

void test_a_transient_without_the_key_breathes_its_preview() {
  Surfaces s;
  BreathingExpression e(&s.base);
  swellOn(e, s, /*transient=*/true, 0);
  e.trigger();
  for (uint32_t now = 1000; now < 1000 + 5 * kSwellMs; now += kFrameMs) frame(e, s, now);
  TEST_ASSERT_FALSE(e.isAnimationComplete());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_a_transient_swell_completes_after_one_swell);
  RUN_TEST(test_a_swell_ends_dark);
  RUN_TEST(test_a_swell_below_the_floor_clamps_to_it);
  RUN_TEST(test_a_configured_breathing_with_the_key_keeps_its_cycle);
  RUN_TEST(test_a_transient_without_the_key_breathes_its_preview);
  return UNITY_END();
}
