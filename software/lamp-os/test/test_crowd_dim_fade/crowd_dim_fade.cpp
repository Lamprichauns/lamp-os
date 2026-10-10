// Native pin for the crowd-dim fade and its home-mode target: crowd-dim lifts
// to 1.0 while home mode is active with social off, and the lift ramps over
// the fade it was given rather than snapping.

#include <unity.h>

#include "core/crowd_dim_fade.hpp"

using namespace lamp;

void setUp() {}
void tearDown() {}

void test_target_lifts_only_in_home_with_social_off() {
  TEST_ASSERT_EQUAL_FLOAT(1.0f, crowdDimTarget(0.5f, true, true));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, crowdDimTarget(0.5f, true, false));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, crowdDimTarget(0.5f, false, true));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, crowdDimTarget(0.5f, false, false));
}

void test_new_target_ramps_over_its_fade() {
  CrowdDimFade f;
  TEST_ASSERT_EQUAL_FLOAT(1.0f, f.at(1.0f, 80, 0));
  f.at(0.5f, 80, 1000);
  TEST_ASSERT_EQUAL_FLOAT(0.5f, f.at(0.5f, 80, 1080));
  TEST_ASSERT_FALSE(f.fading(1080));

  // Home mode lifts the dim: a 1 s ramp back to 1.0, never a snap.
  TEST_ASSERT_EQUAL_FLOAT(0.5f, f.at(1.0f, 1000, 2000));
  TEST_ASSERT_TRUE(f.fading(2000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, f.at(1.0f, 1000, 2500));
  TEST_ASSERT_TRUE(f.fading(2500));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, f.at(1.0f, 1000, 3000));
  TEST_ASSERT_FALSE(f.fading(3000));
}

void test_retarget_mid_fade_glides_from_current_value() {
  CrowdDimFade f;
  f.at(0.0f, 1000, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, f.at(0.0f, 1000, 500));
  // Reversal at 0.5 starts from 0.5, not from either end.
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, f.at(1.0f, 1000, 500));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, f.at(1.0f, 1000, 1000));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_target_lifts_only_in_home_with_social_off);
  RUN_TEST(test_new_target_ramps_over_its_fade);
  RUN_TEST(test_retarget_mid_fade_glides_from_current_value);
  return UNITY_END();
}
