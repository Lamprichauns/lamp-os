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

// A send with no credit free is dropped and counted, so a mesh resend lost
// to contention shows up in heap.get and the trend line.
void test_a_send_without_a_credit_is_counted() {
  TxCredit c;
  for (uint16_t i = 0; i < TxCredit::kMaxInFlight; ++i) TEST_ASSERT_TRUE(c.tryAcquire(0));
  TEST_ASSERT_EQUAL_UINT32(0, c.refused());
  TEST_ASSERT_EQUAL(-1, submitWithCredit(c, 10, -1, [] { return 0; }));
  TEST_ASSERT_FALSE(c.tryAcquire(11));
  TEST_ASSERT_EQUAL_UINT32(2, c.refused());
}

// Every credit lost and nothing sent since: a caller that asks available()
// before it submits (the cascade drain) gets the reset, rather than waiting
// on a send that never comes.
void test_a_full_leak_on_a_quiet_link_frees_up_for_a_waiting_caller() {
  TxCredit c;
  for (uint16_t i = 0; i < TxCredit::kMaxInFlight; ++i) TEST_ASSERT_TRUE(c.tryAcquire(100));
  TEST_ASSERT_FALSE(c.available(100 + TxCredit::kStallMs - 1));
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
  TEST_ASSERT_TRUE(c.available(100 + TxCredit::kStallMs));
  TEST_ASSERT_EQUAL_UINT32(1, c.stalls());
  TEST_ASSERT_EQUAL_UINT16(0, c.inFlight());
}

// A credit whose callback is lost while other frames keep completing stays
// out, and comes back once the link goes quiet for kStallMs.
void test_a_credit_leaked_during_traffic_comes_back_once_quiet() {
  TxCredit c;
  TEST_ASSERT_TRUE(c.tryAcquire(1000));  // its callback never comes
  uint32_t t = 1100;
  for (; t < 1000 + 3 * TxCredit::kStallMs; t += 100) {
    TEST_ASSERT_TRUE(c.tryAcquire(t));
    c.complete(t + 5);
  }
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
  c.available(t + TxCredit::kStallMs - 200);
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
  TEST_ASSERT_TRUE(c.available(t + TxCredit::kStallMs));
  TEST_ASSERT_EQUAL_UINT16(0, c.inFlight());
  TEST_ASSERT_EQUAL_UINT32(1, c.stalls());
}

// Sustained traffic with every callback arriving never holds more than
// kMaxInFlight credits, and never resets.
void test_sustained_traffic_stays_inside_the_bound() {
  TxCredit c;
  uint32_t t = 0;
  for (int round = 0; round < 1000; ++round, t += 3) {
    while (c.tryAcquire(t)) {
    }
    TEST_ASSERT_EQUAL_UINT16(TxCredit::kMaxInFlight, c.inFlight());
    c.complete(t + 1);
    c.complete(t + 2);
  }
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
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

// The submit path EspNowLink runs: a credit per accepted frame, returned at
// once when the driver refuses, and no driver call without one.
void test_submit_takes_and_returns_credits() {
  TxCredit c;
  int calls = 0;
  TEST_ASSERT_EQUAL(0, lamp::submitWithCredit(c, 0, 99, [&] { ++calls; return 0; }));
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
  TEST_ASSERT_EQUAL(7, lamp::submitWithCredit(c, 0, 99, [&] { ++calls; return 7; }));
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
  for (uint16_t i = 1; i < TxCredit::kMaxInFlight; ++i) {
    lamp::submitWithCredit(c, 0, 99, [&] { ++calls; return 0; });
  }
  const int before = calls;
  TEST_ASSERT_EQUAL(99, lamp::submitWithCredit(c, 0, 99, [&] { ++calls; return 0; }));
  TEST_ASSERT_EQUAL(before, calls);
  c.complete(1);
  TEST_ASSERT_EQUAL(0, lamp::submitWithCredit(c, 1, 99, [&] { ++calls; return 0; }));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_bound_holds_until_a_frame_completes);
  RUN_TEST(test_refused_submit_returns_its_credit);
  RUN_TEST(test_a_credit_leaked_during_traffic_comes_back_once_quiet);
  RUN_TEST(test_sustained_traffic_stays_inside_the_bound);
  RUN_TEST(test_a_full_leak_on_a_quiet_link_frees_up_for_a_waiting_caller);
  RUN_TEST(test_a_send_without_a_credit_is_counted);
  RUN_TEST(test_lost_completions_reset_after_the_stall_window);
  RUN_TEST(test_extra_completions_never_underflow);
  RUN_TEST(test_partial_leak_resets);
  RUN_TEST(test_submit_takes_and_returns_credits);
  RUN_TEST(test_a_completion_stamped_ahead_is_not_a_stall);
  RUN_TEST(test_first_frame_after_idle_is_not_a_stall);
  return UNITY_END();
}
