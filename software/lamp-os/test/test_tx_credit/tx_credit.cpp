// Native tests for TxCredit, the bound on ESP-NOW frames queued in the WiFi
// driver: sends past the bound are refused, completions return credits, and
// a lost completion cannot block sending forever.

#include <unity.h>

#include "components/network/transport/tx_credit.hpp"

using lamp::TxCredit;

void setUp(void) {}
void tearDown(void) {}

void test_bound_holds_until_a_frame_completes() {
  TxCredit c;
  for (uint16_t i = 0; i < TxCredit::kMaxInFlight; ++i) TEST_ASSERT_TRUE(c.tryAcquire(0));
  TEST_ASSERT_FALSE(c.tryAcquire(10));
  c.complete(20);
  TEST_ASSERT_TRUE(c.tryAcquire(21));
  TEST_ASSERT_FALSE(c.tryAcquire(22));
}

void test_refused_submit_returns_its_credit() {
  TxCredit c;
  TEST_ASSERT_TRUE(c.tryAcquire(0));
  c.cancel();
  TEST_ASSERT_EQUAL_UINT16(0, c.inFlight());
}

void test_lost_completions_reset_after_the_stall_window() {
  TxCredit c;
  for (uint16_t i = 0; i < TxCredit::kMaxInFlight; ++i) c.tryAcquire(1000);
  TEST_ASSERT_FALSE(c.tryAcquire(1000 + TxCredit::kStallMs - 1));
  TEST_ASSERT_TRUE(c.tryAcquire(1000 + TxCredit::kStallMs + 1));
  TEST_ASSERT_EQUAL_UINT32(1, c.stalls());
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
}

// A few lost callbacks with credits still free would otherwise shrink the
// bound for good; any credit outstanding with no completion for the window
// resets.
void test_partial_leak_resets() {
  TxCredit c;
  c.tryAcquire(1000);
  c.tryAcquire(1000);
  TEST_ASSERT_TRUE(c.tryAcquire(1000 + TxCredit::kStallMs - 1));
  TEST_ASSERT_EQUAL_UINT16(3, c.inFlight());
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
  TEST_ASSERT_TRUE(c.tryAcquire(1000 + TxCredit::kStallMs));
  TEST_ASSERT_EQUAL_UINT32(1, c.stalls());
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
}

// The WiFi task can stamp a completion a little after the loop read millis();
// that newer stamp must not read as a stall from the loop's older clock.
void test_a_completion_stamped_ahead_is_not_a_stall() {
  TxCredit c;
  c.tryAcquire(5000);
  c.tryAcquire(5000);
  c.complete(5010);
  TEST_ASSERT_TRUE(c.tryAcquire(5005));
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
  c.complete(4000);  // an older stamp never moves the clock back
  TEST_ASSERT_TRUE(c.tryAcquire(5010 + TxCredit::kStallMs - 1));
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
}

// A frame going out after an idle spell starts the stall clock afresh.
void test_first_frame_after_idle_is_not_a_stall() {
  TxCredit c;
  c.tryAcquire(0);
  c.complete(10);
  TEST_ASSERT_TRUE(c.tryAcquire(100000));
  TEST_ASSERT_TRUE(c.tryAcquire(100001));
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
}

void test_extra_completions_never_underflow() {
  TxCredit c;
  c.complete(0);
  c.complete(0);
  TEST_ASSERT_EQUAL_UINT16(0, c.inFlight());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_bound_holds_until_a_frame_completes);
  RUN_TEST(test_refused_submit_returns_its_credit);
  RUN_TEST(test_lost_completions_reset_after_the_stall_window);
  RUN_TEST(test_extra_completions_never_underflow);
  RUN_TEST(test_partial_leak_resets);
  RUN_TEST(test_a_completion_stamped_ahead_is_not_a_stall);
  RUN_TEST(test_first_frame_after_idle_is_not_a_stall);
  return UNITY_END();
}
