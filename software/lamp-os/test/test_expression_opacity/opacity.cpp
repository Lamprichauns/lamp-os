#include <unity.h>

#include "../../src/core/animated_behavior.cpp"
#include "../../src/expressions/expression.cpp"

using namespace lamp;

namespace lamp {
// Probe never triggers, but expression.cpp's trigger()/shouldAffectBuffer()
// still need these production globals to link.
OverrideAggregate overrides;
void ExpressionManager::onExpressionFired(Expression*) {}
}  // namespace lamp

namespace {
class Probe : public Expression {
 public:
  using Expression::Expression;
  const ExpressionDescriptor& descriptor() const override { return d_; }
  void configureFromParameters(const ExpressionParams& p) override {
    configureOpacity(p);
  }
  uint8_t opacity() const { return opacityPct_; }
  void draw() override {}

  // Forward to the base easing helpers under test.
  void configureEasingParam(const ExpressionParams& p) { configureEasing(p, 0); }
  Easing currentEasing() const { return easing_; }
  float scale() { return wispDimScale(); }
  void rollEasing() { if (easingRaw_ == (uint32_t)Easing::Random) easing_ = randomEasing(rng); }
 protected:
  void onTrigger() override {}
 private:
  ExpressionDescriptor d_{};
};
}  // namespace

void test_opacity_default_is_100() {
  Probe e(nullptr, 1);
  e.configureFromParameters({});
  TEST_ASSERT_EQUAL_UINT8(100, e.opacity());
}

void test_opacity_reads_param() {
  Probe e(nullptr, 1);
  e.configureFromParameters({{"opacity", 40}});
  TEST_ASSERT_EQUAL_UINT8(40, e.opacity());
}

void test_opacity_clamps_over_100() {
  Probe e(nullptr, 1);
  e.configureFromParameters({{"opacity", 250}});
  TEST_ASSERT_EQUAL_UINT8(100, e.opacity());
}

void test_opacity_floors_under_10() {
  Probe e(nullptr, 1);
  e.configureFromParameters({{"opacity", 0}});
  TEST_ASSERT_EQUAL_UINT8(10, e.opacity());
  e.configureFromParameters({{"opacity", 3}});
  TEST_ASSERT_EQUAL_UINT8(10, e.opacity());
}

void test_random_easing_resolves_to_concrete() {
  Probe e(nullptr, 1);
  e.configureEasingParam({{"easing", (uint32_t)Easing::Random}});
  TEST_ASSERT_TRUE(e.currentEasing() != Easing::Random);
  e.rollEasing();
  TEST_ASSERT_TRUE(e.currentEasing() != Easing::Random);
}

// A retired expression fades its contribution to nothing, then reports done.
void test_retire_fades_contribution_then_reports_retired() {
  set_mock_millis(5000);
  Probe e(nullptr, 1);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, e.scale());
  TEST_ASSERT_FALSE(e.retired());

  e.retire();
  TEST_ASSERT_EQUAL_FLOAT(1.0f, e.scale());
  set_mock_millis(5000 + Expression::kRetireFadeMs / 2);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, e.scale());
  TEST_ASSERT_FALSE(e.retired());

  // A repeat call does not restart the fade.
  e.retire();
  set_mock_millis(5000 + Expression::kRetireFadeMs);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, e.scale());
  TEST_ASSERT_TRUE(e.retired());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_opacity_default_is_100);
  RUN_TEST(test_opacity_reads_param);
  RUN_TEST(test_opacity_clamps_over_100);
  RUN_TEST(test_opacity_floors_under_10);
  RUN_TEST(test_random_easing_resolves_to_concrete);
  RUN_TEST(test_retire_fades_contribution_then_reports_retired);
  return UNITY_END();
}
