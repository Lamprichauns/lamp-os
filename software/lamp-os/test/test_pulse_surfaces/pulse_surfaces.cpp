// Drives the real PulseExpression over native frame buffers with a mock clock:
// single-surface output pinned to golden hashes, and the both-surface pulse
// running base then shade as one strip.

#include <unity.h>

#include <cstdint>
#include <vector>

#include "../../src/core/animated_behavior.cpp"
#include "../../src/core/frame_buffer.cpp"
#include "../../src/expressions/expression.cpp"
#include "../../src/expressions/pulse/pulse_expression.cpp"

using namespace lamp;

namespace lamp {
OverrideAggregate overrides;
}  // namespace lamp

namespace {

int g_fired = 0;

constexpr uint8_t kShadePx = 38;
constexpr uint8_t kBasePx = 35;
constexpr uint32_t kFrameMs = 16;
const Color kBackground{0, 0, 0, 0};
const Color kPulse{200, 40, 10, 0};

struct Rig {
  FrameBuffer shade;
  FrameBuffer base;
  BehaviorContext ctx;

  Rig() {
    shade.pixelCount = kShadePx;
    shade.buffer.assign(kShadePx, kBackground);
    base.pixelCount = kBasePx;
    base.buffer.assign(kBasePx, kBackground);
    ctx.expressionFrameBuffers = {&shade, &base};
    ctx.expressionManager = reinterpret_cast<ExpressionManager*>(this);
    overrides.base.setOperatorEditing(false);
    overrides.shade.setOperatorEditing(false);
    g_fired = 0;
  }

  void clear() {
    shade.buffer.assign(kShadePx, kBackground);
    base.buffer.assign(kBasePx, kBackground);
  }
};

uint32_t fnv(uint32_t h, const std::vector<Color>& buf) {
  for (const Color& c : buf) {
    for (uint8_t b : {c.r, c.g, c.b, c.w}) {
      h ^= b;
      h *= 16777619u;
    }
  }
  return h;
}

struct RunResult {
  uint32_t hash = 2166136261u;
  uint32_t frames = 0;
};

// One compositor tick per frame: background, control(), draw() while playing.
RunResult run(Rig& rig, PulseExpression& p, uint32_t maxFrames) {
  RunResult r;
  uint32_t now = 1000;
  set_mock_millis(now);
  TEST_ASSERT_TRUE(p.trigger());
  for (uint32_t f = 0; f < maxFrames; ++f) {
    now += kFrameMs;
    set_mock_millis(now);
    rig.clear();
    p.control();
    if (p.animationState == STOPPED) break;
    p.draw();
    r.hash = fnv(fnv(r.hash, rig.shade.buffer), rig.base.buffer);
    ++r.frames;
  }
  return r;
}

void build(Rig& rig, PulseExpression& p, ExpressionTarget target,
           const ExpressionParams& params) {
  p.setBehaviorContext(&rig.ctx);
  p.configure({kPulse}, 300, 900, target);
  p.autoTriggerEnabled = false;
  p.configureFromParameters(params);
}

}  // namespace

namespace lamp {
void ExpressionManager::onExpressionFired(Expression*) { ++g_fired; }
}  // namespace lamp

void setUp() {}
void tearDown() {}

// Golden hashes recorded from the per-surface Pulse before TARGET_BOTH joined
// the surfaces. A single-surface pulse must render byte-identically.
void test_shade_only_unchanged() {
  Rig rig;
  PulseExpression p(&rig.shade);
  build(rig, p, TARGET_SHADE, {{"pulseSpeed", 6}, {"size", 4}});
  const RunResult r = run(rig, p, 5000);
  TEST_ASSERT_EQUAL_UINT32(659u, r.frames);
  TEST_ASSERT_EQUAL_HEX32(0xAC8B11CAu, r.hash);
}

void test_base_only_unchanged() {
  Rig rig;
  PulseExpression p(&rig.base);
  build(rig, p, TARGET_BASE, {{"pulseSpeed", 2}, {"size", 9}, {"easing", 1}});
  const RunResult r = run(rig, p, 5000);
  TEST_ASSERT_EQUAL_UINT32(514u, r.frames);
  TEST_ASSERT_EQUAL_HEX32(0xEA15BAEDu, r.hash);
}

void test_shade_region_continuous_unchanged() {
  Rig rig;
  PulseExpression p(&rig.shade);
  build(rig, p, TARGET_SHADE,
        {{"pulseSpeed", 4}, {"fullStrip", 0}, {"posMin", 5}, {"posMax", 30},
         {"loop", 1}, {"easing", 2}, {"opacity", 60}});
  const RunResult r = run(rig, p, 5000);
  TEST_ASSERT_EQUAL_UINT32(1027u, r.frames);
  TEST_ASSERT_EQUAL_HEX32(0x164D9945u, r.hash);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_shade_only_unchanged);
  RUN_TEST(test_base_only_unchanged);
  RUN_TEST(test_shade_region_continuous_unchanged);
  return UNITY_END();
}
