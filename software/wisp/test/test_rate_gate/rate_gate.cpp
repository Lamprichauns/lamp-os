#include <unity.h>

#include "util/rate_gate.hpp"

using wisp::RateGate;

void setUp() {}
void tearDown() {}

void test_first_request_runs() {
  RateGate g(1000);
  TEST_ASSERT_TRUE(g.request(5));
  TEST_ASSERT_FALSE(g.due(5000));
}

// A burst inside the interval runs once at the leading edge and once at the
// trailing edge, so the last change lands.
void test_burst_coalesces_with_trailing_edge() {
  RateGate g(1000);
  TEST_ASSERT_TRUE(g.request(0));
  TEST_ASSERT_FALSE(g.request(100));
  TEST_ASSERT_FALSE(g.request(900));
  TEST_ASSERT_FALSE(g.due(999));
  TEST_ASSERT_TRUE(g.due(1000));
  TEST_ASSERT_FALSE(g.due(1001));
}

void test_spaced_requests_all_run() {
  RateGate g(1000);
  TEST_ASSERT_TRUE(g.request(0));
  TEST_ASSERT_TRUE(g.request(1000));
  TEST_ASSERT_TRUE(g.request(2500));
  TEST_ASSERT_FALSE(g.due(10000));
}

// The trailing run starts a fresh interval.
void test_trailing_run_restarts_interval() {
  RateGate g(1000);
  g.request(0);
  g.request(500);
  TEST_ASSERT_TRUE(g.due(1200));
  TEST_ASSERT_FALSE(g.request(1300));
  TEST_ASSERT_TRUE(g.due(2200));
}

void test_reset_drops_held_and_reopens() {
  RateGate g(1000);
  g.request(0);
  g.request(10);
  g.reset();
  TEST_ASSERT_FALSE(g.due(5000));
  TEST_ASSERT_TRUE(g.request(20));
}

void test_millis_wrap() {
  RateGate g(1000);
  TEST_ASSERT_TRUE(g.request(0xFFFFFF00u));
  TEST_ASSERT_FALSE(g.request(0x00000010u));
  TEST_ASSERT_TRUE(g.due(0xFFFFFF00u + 1000u));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_first_request_runs);
  RUN_TEST(test_burst_coalesces_with_trailing_edge);
  RUN_TEST(test_spaced_requests_all_run);
  RUN_TEST(test_trailing_run_restarts_interval);
  RUN_TEST(test_reset_drops_held_and_reopens);
  RUN_TEST(test_millis_wrap);
  return UNITY_END();
}
