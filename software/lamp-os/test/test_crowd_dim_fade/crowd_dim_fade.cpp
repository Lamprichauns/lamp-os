// Native pin for CrowdDim: the factor follows the engine, lifts to 1.0 while
// home mode ignores peers, ramps over 1 s when that flips and 80 ms for an
// ordinary commit, glides on a mid-ramp retarget, and stays settled across a
// millis() wrap.

#include <unity.h>

#include <cstdint>

#include "core/crowd_dim_fade.hpp"

using namespace lamp;

void setUp() {}
void tearDown() {}

// Engine at 0.5, settled at t=0..80.
static CrowdDim dimmed() {
  CrowdDim d;
  d.at(0.5f, false, 0);
  d.at(0.5f, false, kCrowdDimFadeMs);
  return d;
}

void test_ordinary_commit_ramps_over_80ms() {
  CrowdDim d;
  TEST_ASSERT_EQUAL_FLOAT(1.0f, d.at(1.0f, false, 0));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, d.at(0.5f, false, 1000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, d.at(0.5f, false, 1000 + kCrowdDimFadeMs / 2));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, d.at(0.5f, false, 1000 + kCrowdDimFadeMs));
}

// Home mode ignoring peers lifts the dim to 1.0 over a second, never a snap.
void test_ignoring_peers_lifts_over_a_second() {
  CrowdDim d = dimmed();
  TEST_ASSERT_EQUAL_FLOAT(0.5f, d.at(0.5f, true, 2000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, d.at(0.5f, true, 2000 + kCrowdDimHomeFadeMs / 2));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, d.at(0.5f, true, 2000 + kCrowdDimHomeFadeMs));
  // The engine moving meanwhile changes nothing.
  TEST_ASSERT_EQUAL_FLOAT(1.0f, d.at(0.3f, true, 4000));
}

// Leaving home eases back down to the engine's factor over a second too.
void test_ending_ignore_restores_over_a_second() {
  CrowdDim d;
  d.at(0.5f, true, 0);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, d.at(0.5f, false, 10000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, d.at(0.5f, false, 10000 + kCrowdDimHomeFadeMs / 2));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, d.at(0.5f, false, 10000 + kCrowdDimHomeFadeMs));
}

void test_retarget_mid_ramp_glides_from_current_value() {
  CrowdDim d = dimmed();
  d.at(0.5f, true, 1000);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, d.at(0.5f, true, 1500));
  // Home mode ends mid-lift: the ramp turns around at 0.75.
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, d.at(0.5f, false, 1500));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.625f, d.at(0.5f, false, 2000));
}

// fading() reports every pass of the ramp plus the one after it ends, then
// stops.
void test_fading_covers_the_landing_pass() {
  CrowdDim d = dimmed();
  d.fading(kCrowdDimFadeMs);  // the initial ramp's landing pass
  TEST_ASSERT_FALSE(d.fading(500));
  d.at(0.5f, true, 1000);
  TEST_ASSERT_TRUE(d.fading(1500));
  TEST_ASSERT_TRUE(d.fading(1000 + kCrowdDimHomeFadeMs));
  TEST_ASSERT_FALSE(d.fading(1000 + kCrowdDimHomeFadeMs + 1));
}

// 2^32 ms after a ramp started, now - start reads as inside the ramp again; a
// settled ramp stays at its target. Each pass calls at() then fading(), as the
// loop does.
void test_settled_ramp_survives_millis_wrap() {
  CrowdDim d = dimmed();
  d.fading(kCrowdDimFadeMs);
  d.at(0.5f, true, 1000);
  d.at(0.5f, true, 1000 + kCrowdDimHomeFadeMs);  // settles
  TEST_ASSERT_TRUE(d.fading(1000 + kCrowdDimHomeFadeMs));  // landing pass
  const uint32_t wrapped = 1000 + kCrowdDimHomeFadeMs / 2;  // 2^32 ms later
  TEST_ASSERT_EQUAL_FLOAT(1.0f, d.at(0.5f, true, wrapped));
  TEST_ASSERT_FALSE(d.fading(wrapped));
}

// A pass timed just before the ramp began (the loop's `now` against a ramp
// started inside the brightness tick) reads as the ramp's start.
void test_timestamp_before_ramp_start_reads_as_start() {
  CrowdDim d = dimmed();
  d.fading(kCrowdDimFadeMs);
  d.at(0.5f, true, 1000);
  TEST_ASSERT_TRUE(d.fading(990));
  TEST_ASSERT_TRUE(d.fading(1500));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, d.at(0.5f, true, 990));
}

// Leaving home starts a 1 s ramp from 1.0 to the engine's 0.5; a crowd commit
// to 0.48 at 300 ms keeps the remaining 700 ms instead of landing in 80.
void test_commit_during_home_ramp_keeps_its_remaining_time() {
  CrowdDim d;
  d.at(0.5f, true, 0);
  d.at(0.5f, false, 10000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.85f, d.at(0.5f, false, 10300));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.85f, d.at(0.48f, false, 10300));
  // The 1 s curve to 0.5 reads 0.675 at 650 ms; the commit stays near it.
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.675f, d.at(0.48f, false, 10650));
  TEST_ASSERT_EQUAL_FLOAT(0.48f, d.at(0.48f, false, 11000));
}

// A second commit while the first still runs on the home ramp's time keeps
// the remaining time too: 0.744 at 500 ms, then 500 ms more to 0.46.
void test_chained_commits_keep_the_home_ramps_time() {
  CrowdDim d;
  d.at(0.5f, true, 0);
  d.at(0.5f, false, 10000);
  d.at(0.48f, false, 10300);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.744f, d.at(0.46f, false, 10500));
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.687f, d.at(0.46f, false, 10600));
  TEST_ASSERT_EQUAL_FLOAT(0.46f, d.at(0.46f, false, 11000));
}

// A commit in the home ramp's last 80 ms still takes the ordinary 80.
void test_commit_late_in_home_ramp_takes_at_least_80ms() {
  CrowdDim d;
  d.at(0.5f, true, 0);
  d.at(0.5f, false, 10000);
  d.at(0.48f, false, 10950);
  TEST_ASSERT_TRUE(d.at(0.48f, false, 11000) > 0.481f);
  TEST_ASSERT_EQUAL_FLOAT(0.48f, d.at(0.48f, false, 10950 + kCrowdDimFadeMs));
}

// Once the home ramp has landed, a commit takes the ordinary 80 ms.
void test_commit_after_home_ramp_takes_80ms() {
  CrowdDim d;
  d.at(0.5f, true, 0);
  d.at(0.5f, false, 10000);
  d.at(0.5f, false, 11000);
  d.at(0.48f, false, 12000);
  TEST_ASSERT_EQUAL_FLOAT(0.48f, d.at(0.48f, false, 12000 + kCrowdDimFadeMs));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ordinary_commit_ramps_over_80ms);
  RUN_TEST(test_ignoring_peers_lifts_over_a_second);
  RUN_TEST(test_ending_ignore_restores_over_a_second);
  RUN_TEST(test_retarget_mid_ramp_glides_from_current_value);
  RUN_TEST(test_fading_covers_the_landing_pass);
  RUN_TEST(test_settled_ramp_survives_millis_wrap);
  RUN_TEST(test_timestamp_before_ramp_start_reads_as_start);
  RUN_TEST(test_commit_during_home_ramp_keeps_its_remaining_time);
  RUN_TEST(test_commit_after_home_ramp_takes_80ms);
  RUN_TEST(test_chained_commits_keep_the_home_ramps_time);
  RUN_TEST(test_commit_late_in_home_ramp_takes_at_least_80ms);
  return UNITY_END();
}
