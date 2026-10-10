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
  TEST_ASSERT_EQUAL_UINT32(TxCredit::kMaxInFlight, c.stalls());
  TEST_ASSERT_EQUAL_UINT16(0, c.inFlight());
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
  TEST_ASSERT_EQUAL_UINT32(TxCredit::kMaxInFlight, c.stalls());
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
}

// A lost callback while other frames keep completing: that one credit is
// taken back once it is kStallMs old, so the bound never shrinks for good
// however busy the link stays.
void test_a_leaked_credit_comes_back_while_traffic_flows() {
  TxCredit c;
  TEST_ASSERT_TRUE(c.tryAcquire(1000));  // its callback never comes
  for (uint32_t t = 1100; t < 1000 + TxCredit::kStallMs; t += 100) {
    TEST_ASSERT_TRUE(c.tryAcquire(t));
    TEST_ASSERT_TRUE(c.tryAcquire(t + 1));
    c.complete(t + 5);
    c.complete(t + 6);
  }
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
  TEST_ASSERT_TRUE(c.available(1000 + TxCredit::kStallMs));
  TEST_ASSERT_EQUAL_UINT32(1, c.stalls());
  TEST_ASSERT_EQUAL_UINT16(0, c.inFlight());
}

// A credit stamped by the WiFi task a little after the loop read millis()
// reads as fresh from the loop's older clock.
void test_a_credit_stamped_ahead_is_not_stale() {
  TxCredit c;
  TEST_ASSERT_TRUE(c.tryAcquire(5010));
  TEST_ASSERT_TRUE(c.available(5005));
  TEST_ASSERT_EQUAL_UINT16(1, c.inFlight());
  TEST_ASSERT_EQUAL_UINT32(0, c.stalls());
}

// A frame going out after an idle spell is fresh.
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
  RUN_TEST(test_a_full_leak_on_a_quiet_link_frees_up_for_a_waiting_caller);
  RUN_TEST(test_a_send_without_a_credit_is_counted);
  RUN_TEST(test_lost_completions_reset_after_the_stall_window);
  RUN_TEST(test_extra_completions_never_underflow);
  RUN_TEST(test_a_leaked_credit_comes_back_while_traffic_flows);
  RUN_TEST(test_submit_takes_and_returns_credits);
  RUN_TEST(test_a_credit_stamped_ahead_is_not_stale);
  RUN_TEST(test_first_frame_after_idle_is_not_a_stall);
  return UNITY_END();
}
