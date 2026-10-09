#include <unity.h>

#include "osc/osc_freshness.hpp"

using wisp::OscFreshness;

void setUp() {}
void tearDown() {}

void test_first_palette_is_live_edge() {
  OscFreshness f;
  TEST_ASSERT_FALSE(f.live());
  TEST_ASSERT_TRUE(f.onPalette(100));
  TEST_ASSERT_TRUE(f.live());
  TEST_ASSERT_FALSE(f.onPalette(200));
}

void test_stale_after_gap_once() {
  OscFreshness f;
  f.onPalette(1000);
  TEST_ASSERT_FALSE(f.tick(1000 + OscFreshness::kStaleMs - 1));
  TEST_ASSERT_TRUE(f.live());
  TEST_ASSERT_TRUE(f.tick(1000 + OscFreshness::kStaleMs));
  TEST_ASSERT_FALSE(f.live());
  TEST_ASSERT_FALSE(f.tick(1000 + 2 * OscFreshness::kStaleMs));
}

// A palette after a gap resumes on its own: the stale -> live edge fires again.
void test_resumes_after_gap() {
  OscFreshness f;
  f.onPalette(0);
  f.tick(OscFreshness::kStaleMs);
  TEST_ASSERT_TRUE(f.onPalette(OscFreshness::kStaleMs + 5));
  TEST_ASSERT_TRUE(f.live());
}

void test_steady_stream_never_stales() {
  OscFreshness f;
  for (uint32_t t = 0; t < 5 * OscFreshness::kStaleMs; t += 500) {
    f.onPalette(t);
    TEST_ASSERT_FALSE(f.tick(t + 499));
  }
  TEST_ASSERT_TRUE(f.live());
}

void test_millis_wrap() {
  OscFreshness f;
  f.onPalette(0xFFFFFF00u);
  TEST_ASSERT_FALSE(f.tick(0x00000100u));
  TEST_ASSERT_TRUE(f.tick(0xFFFFFF00u + OscFreshness::kStaleMs));
}

void test_reset_drops_live_without_edge() {
  OscFreshness f;
  f.onPalette(0);
  f.reset();
  TEST_ASSERT_FALSE(f.live());
  TEST_ASSERT_FALSE(f.tick(OscFreshness::kStaleMs * 2));
  TEST_ASSERT_TRUE(f.onPalette(1));
}

// Straylight restates a still palette every 10 s over UDP; that cadence
// must never read as stale.
void test_ten_second_restate_stays_live() {
  OscFreshness f;
  for (uint32_t t = 0; t <= 120000; t += 10000) {
    f.onPalette(t);
    TEST_ASSERT_FALSE(f.tick(t + 9999));
  }
  TEST_ASSERT_TRUE(f.live());
}

void test_blank_palette_detected() {
  const float zeros[6] = {0, 0, 0, 0, 0, 0};
  TEST_ASSERT_TRUE(wisp::isBlankPalette(zeros, 6));
  const float negative[3] = {-1.0f, 0.0f, -0.5f};
  TEST_ASSERT_TRUE(wisp::isBlankPalette(negative, 3));
  const float oneLit[6] = {0, 0, 0, 0, 0.01f, 0};
  TEST_ASSERT_FALSE(wisp::isBlankPalette(oneLit, 6));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_first_palette_is_live_edge);
  RUN_TEST(test_stale_after_gap_once);
  RUN_TEST(test_resumes_after_gap);
  RUN_TEST(test_steady_stream_never_stales);
  RUN_TEST(test_millis_wrap);
  RUN_TEST(test_reset_drops_live_without_edge);
  RUN_TEST(test_ten_second_restate_stays_live);
  RUN_TEST(test_blank_palette_detected);
  return UNITY_END();
}
