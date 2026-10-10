// Native tests for the save-retry backoff: failures space out exponentially to
// a ceiling and a success resets, so a failing save never retries hot.

#include <unity.h>

#include "util/retry_backoff.hpp"

using lamp::RetryBackoff;

void setUp(void) {}
void tearDown(void) {}

void test_idle_backoff_is_due() {
  RetryBackoff b(5000, 60000);
  TEST_ASSERT_TRUE(b.due(0));
  TEST_ASSERT_FALSE(b.pending());
}

void test_failures_double_to_the_ceiling() {
  RetryBackoff b(5000, 60000);
  const uint32_t expected[] = {5000, 10000, 20000, 40000, 60000, 60000};
  uint32_t now = 1000;
  for (uint32_t want : expected) {
    b.fail(now);
    TEST_ASSERT_EQUAL_UINT32(want, b.waitMs());
    TEST_ASSERT_FALSE(b.due(now + want - 1));
    TEST_ASSERT_TRUE(b.due(now + want));
    now += want;
  }
}

void test_success_resets() {
  RetryBackoff b(5000, 60000);
  b.fail(0);
  b.fail(5000);
  b.succeed();
  TEST_ASSERT_TRUE(b.due(5001));
  b.fail(6000);
  TEST_ASSERT_EQUAL_UINT32(5000, b.waitMs());
}

void test_due_across_millis_wrap() {
  RetryBackoff b(5000, 60000);
  b.fail(0xFFFFF000u);
  TEST_ASSERT_FALSE(b.due(0x00000100u));
  TEST_ASSERT_TRUE(b.due(0x00000400u));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_idle_backoff_is_due);
  RUN_TEST(test_failures_double_to_the_ceiling);
  RUN_TEST(test_success_resets);
  RUN_TEST(test_due_across_millis_wrap);
  return UNITY_END();
}
