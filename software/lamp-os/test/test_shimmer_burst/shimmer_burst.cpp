// Native pin for the flicker burst against the real ShimmerExpression: a
// transient burst completes so the transient GC reaps it, only its sparked
// cells ever paint, and a configured auto-triggering flicker carrying the same
// keys stays continuous.

#include <unity.h>

#include <vector>

#include "../../src/core/animated_behavior.cpp"
#include "../../src/core/frame_buffer.cpp"
#include "../../src/expressions/expression.cpp"
#include "../../src/expressions/shimmer/shimmer_expression.cpp"
#include "../../src/util/color.cpp"
#include "../../src/util/fade.cpp"

using namespace lamp;

namespace lamp {
OverrideAggregate overrides;
void ExpressionManager::onExpressionFired(Expression*) {}
}  // namespace lamp

namespace {

constexpr uint8_t kPixels = 24;
constexpr uint32_t kStars = 5;
constexpr uint32_t kBurstMs = 400;
constexpr uint32_t kFrameMs = 16;
const Color kAnchor(120, 60, 0, 0);

struct Surfaces {
  FrameBuffer shade;
  FrameBuffer base;
  BehaviorContext ctx;
  Surfaces() {
    for (FrameBuffer* fb : {&shade, &base}) {
      fb->pixelCount = kPixels;
      fb->buffer.assign(kPixels, kAnchor);
    }
    ctx.expressionFrameBuffers = {&shade, &base};
  }
};

// The anchor carries no blue, and Twinkle's warmth ramp never adds any, so
// blue on the surface can only come from a tint.
const Color kBlueTint(0, 0, 255, 0);

void burstOn(ShimmerExpression& e, Surfaces& s, bool transient,
             std::vector<Color> colors = {}) {
  e.configure(colors, 60, 900, TARGET_SHADE);
  e.configureFromParameters(
      {{"fire", 0}, {"count", kStars}, {"durationMax", kBurstMs}, {"opacity", 100}});
  e.autoTriggerEnabled = !transient;
  e.setBehaviorContext(&s.ctx);
}

// One compositor frame: the layer below repaints the anchor, then the
// expression runs its control and draw.
size_t frame(ShimmerExpression& e, Surfaces& s, uint32_t nowMs) {
  set_mock_millis(nowMs);
  s.shade.buffer.assign(kPixels, kAnchor);
  e.control();
  e.draw();
  size_t touched = 0;
  for (const Color& c : s.shade.buffer) touched += !(c == kAnchor);
  return touched;
}

}  // namespace

void setUp() { set_mock_millis(1000); }
void tearDown() {}

void test_a_transient_burst_completes_for_the_gc() {
  Surfaces s;
  ShimmerExpression e(&s.shade);
  burstOn(e, s, /*transient=*/true);
  TEST_ASSERT_TRUE(e.trigger());
  size_t peak = 0;
  uint32_t now = 1000;
  for (; now < 1000 + kBurstMs; now += kFrameMs) {
    TEST_ASSERT_FALSE(e.isAnimationComplete());
    const size_t touched = frame(e, s, now);
    peak = touched > peak ? touched : peak;
  }
  for (int i = 0; i < 3; ++i) frame(e, s, now += kFrameMs);
  TEST_ASSERT_TRUE(peak > 0);
  // gcTransients reaps on isAnimationComplete(); the 180 s cap is never reached.
  TEST_ASSERT_TRUE(e.isAnimationComplete());
  TEST_ASSERT_TRUE(now - 1000 < 180000);
}

void test_only_sparked_cells_ever_paint() {
  Surfaces s;
  ShimmerExpression e(&s.shade);
  burstOn(e, s, /*transient=*/true);
  e.trigger();
  for (uint32_t now = 1000; now < 1000 + kBurstMs + 4 * kFrameMs; now += kFrameMs) {
    TEST_ASSERT_TRUE(frame(e, s, now) <= kStars);
  }
}

void test_a_configured_flicker_with_burst_keys_stays_continuous() {
  Surfaces s;
  ShimmerExpression e(&s.shade);
  burstOn(e, s, /*transient=*/false);
  e.trigger();
  size_t widest = 0;
  for (uint32_t now = 1000; now < 1000 + 5 * kBurstMs; now += kFrameMs) {
    const size_t touched = frame(e, s, now);
    widest = touched > widest ? touched : widest;
  }
  TEST_ASSERT_FALSE(e.isAnimationComplete());
  TEST_ASSERT_TRUE(e.animationState != STOPPED);
  // Continuous Twinkle paints every cell of its zone.
  TEST_ASSERT_EQUAL_UINT32(kPixels, widest);
}

// The most blue and the most white any pixel reached over the run. The anchor
// carries neither, so either can only come from a colour other than it.
struct Reach {
  uint8_t blue = 0;
  uint8_t white = 0;
};

Reach reach(ShimmerExpression& e, Surfaces& s, uint32_t spanMs) {
  e.trigger();
  Reach r;
  for (uint32_t now = 1000; now < 1000 + spanMs; now += kFrameMs) {
    frame(e, s, now);
    for (const Color& c : s.shade.buffer) {
      r.blue = c.b > r.blue ? c.b : r.blue;
      r.white = c.w > r.white ? c.w : r.white;
    }
  }
  return r;
}

void test_a_burst_paints_its_tint() {
  Surfaces s;
  ShimmerExpression e(&s.shade);
  burstOn(e, s, /*transient=*/true, {kBlueTint});
  TEST_ASSERT_TRUE(reach(e, s, kBurstMs).blue > 0);
}

void test_an_untinted_burst_paints_the_colour_under_it() {
  Surfaces s;
  ShimmerExpression e(&s.shade);
  burstOn(e, s, /*transient=*/true);
  const Reach r = reach(e, s, kBurstMs);
  TEST_ASSERT_EQUAL_UINT8(0, r.blue);
  TEST_ASSERT_EQUAL_UINT8(0, r.white);
}

void test_a_configured_flicker_ignores_a_tint() {
  Surfaces s;
  ShimmerExpression e(&s.shade);
  burstOn(e, s, /*transient=*/false, {kBlueTint});
  TEST_ASSERT_EQUAL_UINT8(0, reach(e, s, 5 * kBurstMs).blue);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_a_transient_burst_completes_for_the_gc);
  RUN_TEST(test_only_sparked_cells_ever_paint);
  RUN_TEST(test_a_configured_flicker_with_burst_keys_stays_continuous);
  RUN_TEST(test_a_burst_paints_its_tint);
  RUN_TEST(test_an_untinted_burst_paints_the_colour_under_it);
  RUN_TEST(test_a_configured_flicker_ignores_a_tint);
  return UNITY_END();
}
