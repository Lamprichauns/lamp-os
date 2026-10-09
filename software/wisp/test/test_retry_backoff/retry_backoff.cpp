#include <unity.h>

#include "util/retry_backoff.hpp"

using wisp::RetryBackoff;

void setUp() {}
void tearDown() {}

void test_unarmed_never_due() {
  RetryBackoff b;
  TEST_ASSERT_FALSE(b.due(0));
  TEST_ASSERT_FALSE(b.due(0xFFFFFFFFu));
}

void test_reset_is_due_at_once() {
  RetryBackoff b;
  b.reset(5000);
  TEST_ASSERT_TRUE(b.due(5000));
}

// 10 s, 20 s, 40 s, 80 s, 160 s, then capped at 300 s.
void test_schedule_doubles_to_cap() {
  RetryBackoff b;
  b.reset(0);
  const uint32_t waits[] = {10000, 20000, 40000, 80000, 160000,
                            300000, 300000};
  uint32_t now = 0;
  for (uint32_t w : waits) {
    TEST_ASSERT_EQUAL_UINT32(w, b.failed(now));
    TEST_ASSERT_FALSE(b.due(now + w - 1));
    TEST_ASSERT_TRUE(b.due(now + w));
    now += w;
  }
}

void test_reset_restarts_schedule() {
  RetryBackoff b;
  b.reset(0);
  b.failed(0);
  b.failed(10000);
  b.failed(30000);
  b.reset(70000);
  TEST_ASSERT_TRUE(b.due(70000));
  b.failed(70000);
  TEST_ASSERT_TRUE(b.due(80000));
  TEST_ASSERT_FALSE(b.due(79999));
}

void test_stop_disarms() {
  RetryBackoff b;
  b.reset(0);
  b.failed(0);
  b.stop();
  TEST_ASSERT_FALSE(b.due(1000000));
}

void test_millis_wrap() {
  RetryBackoff b;
  b.reset(0xFFFFF000u);
  b.failed(0xFFFFF000u);
  TEST_ASSERT_FALSE(b.due(0x00000010u));
  TEST_ASSERT_TRUE(b.due(0xFFFFF000u + RetryBackoff::kInitialMs));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_unarmed_never_due);
  RUN_TEST(test_reset_is_due_at_once);
  RUN_TEST(test_schedule_doubles_to_cap);
  RUN_TEST(test_reset_restarts_schedule);
  RUN_TEST(test_stop_disarms);
  RUN_TEST(test_millis_wrap);
  return UNITY_END();
}
