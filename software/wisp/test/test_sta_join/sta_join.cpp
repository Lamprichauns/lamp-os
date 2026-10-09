#include <unity.h>

#include "net/sta_join.hpp"

using wisp::StaAction;
using wisp::StaJoin;
using State = wisp::StaJoin::State;

void setUp() {}
void tearDown() {}

static void assertAction(StaAction want, StaAction got) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(want), static_cast<int>(got));
}

static void assertState(State want, const StaJoin& j) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(want), static_cast<int>(j.state()));
}

// Nothing scans or joins until the source wants the network.
void test_idle_until_wanted() {
  StaJoin j;
  assertAction(StaAction::None, j.step(0, true));
  assertAction(StaAction::None, j.step(1000000, true));
  assertState(State::Idle, j);
}

// AP absent: each failed mesh-channel scan waits 10, 20, 40 s ... before the
// next, and never starts a join.
void test_absent_ap_backs_off() {
  StaJoin j;
  j.setWanted(true, 0);
  uint32_t now = 0;
  const uint32_t waits[] = {10000, 20000, 40000, 80000, 160000, 300000,
                            300000};
  for (uint32_t w : waits) {
    assertAction(StaAction::StartScan, j.step(now, true));
    assertAction(StaAction::None, j.onScanDone(false, now + 100));
    TEST_ASSERT_EQUAL_UINT32(w, j.lastWaitMs());
    assertAction(StaAction::None, j.step(now + 100 + w - 1, true));
    now += 100 + w;
  }
}

// The AP shows up later: the next scheduled scan finds it and joins.
void test_ap_appears_later_joins() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  j.onScanDone(false, 50);
  assertAction(StaAction::StartScan, j.step(10050, true));
  assertAction(StaAction::Begin, j.onScanDone(true, 10100));
  j.onConnected();
  assertState(State::Linked, j);
  assertAction(StaAction::None, j.step(999999, true));
}

// The latched-status regression: a join just begun is never judged failed
// without an event, however soon the loop looks again.
void test_new_join_not_failed_without_event() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  j.onScanDone(false, 10);
  j.step(10010, true);
  assertAction(StaAction::Begin, j.onScanDone(true, 10020));
  assertAction(StaAction::None, j.step(10025, true));
  assertAction(StaAction::None, j.step(10020 + StaJoin::kJoinTimeoutMs - 1,
                                       true));
  assertState(State::Joining, j);
  assertAction(StaAction::Drop,
               j.step(10020 + StaJoin::kJoinTimeoutMs, true));
  assertState(State::Waiting, j);
}

// Wrong password: the join's disconnect is a failure on backoff; new creds
// restart at once and join.
void test_wrong_password_then_fixed() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  assertAction(StaAction::Begin, j.onScanDone(true, 10));
  j.onDisconnected(500);
  assertState(State::Waiting, j);
  assertAction(StaAction::None, j.step(600, true));
  assertAction(StaAction::None, j.restart(700));
  assertAction(StaAction::StartScan, j.step(700, true));
  assertAction(StaAction::Begin, j.onScanDone(true, 800));
  j.onConnected();
  assertState(State::Linked, j);
}

// A lost link retries after the first backoff step, even after a long run
// of earlier failures.
void test_lost_link_retries_in_ten_seconds() {
  StaJoin j;
  j.setWanted(true, 0);
  uint32_t now = 0;
  for (int i = 0; i < 4; ++i) {
    j.step(now, true);
    j.onScanDone(false, now);
    now += 400000;
  }
  j.step(now, true);
  j.onScanDone(true, now);
  j.onConnected();
  j.onDisconnected(now + 1000);
  assertAction(StaAction::None, j.step(now + 1000 + 9999, true));
  assertAction(StaAction::StartScan, j.step(now + 1000 + 10000, true));
}

// Leaving Osc drops the link and nothing retries.
void test_unwanted_drops_and_stops() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  j.onScanDone(true, 10);
  j.onConnected();
  assertAction(StaAction::Drop, j.setWanted(false, 20));
  assertState(State::Idle, j);
  assertAction(StaAction::None, j.step(1000000, true));
  j.onDisconnected(1000001);
  assertAction(StaAction::None, j.step(2000000, true));
  j.onConnected();
  assertState(State::Idle, j);
}

// The core's own one-shot retry can link after the join was counted failed:
// the link is adopted and no new join starts on top of it.
void test_core_retry_link_adopted() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  j.onScanDone(true, 10);
  j.onDisconnected(100);
  j.onConnected();
  assertState(State::Linked, j);
  assertAction(StaAction::None, j.step(100000, true));
}

void test_scan_timeout_counts_as_failure() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  assertAction(StaAction::None, j.step(StaJoin::kScanTimeoutMs - 1, true));
  assertAction(StaAction::Drop, j.step(StaJoin::kScanTimeoutMs, true));
  assertState(State::Waiting, j);
}

void test_no_creds_goes_idle() {
  StaJoin j;
  j.setWanted(true, 0);
  assertAction(StaAction::None, j.step(0, false));
  assertState(State::Idle, j);
  j.restart(10);
  assertAction(StaAction::StartScan, j.step(10, true));
}

// A wrong-channel guard halts retries until a restart.
void test_halt_until_restart() {
  StaJoin j;
  j.setWanted(true, 0);
  j.step(0, true);
  j.onScanDone(true, 10);
  j.onConnected();
  assertAction(StaAction::Drop, j.halt());
  assertAction(StaAction::None, j.step(10000000, true));
  j.onConnected();
  assertState(State::Halted, j);
  j.restart(20000000);
  assertAction(StaAction::StartScan, j.step(20000000, true));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_idle_until_wanted);
  RUN_TEST(test_absent_ap_backs_off);
  RUN_TEST(test_ap_appears_later_joins);
  RUN_TEST(test_new_join_not_failed_without_event);
  RUN_TEST(test_wrong_password_then_fixed);
  RUN_TEST(test_lost_link_retries_in_ten_seconds);
  RUN_TEST(test_unwanted_drops_and_stops);
  RUN_TEST(test_core_retry_link_adopted);
  RUN_TEST(test_scan_timeout_counts_as_failure);
  RUN_TEST(test_no_creds_goes_idle);
  RUN_TEST(test_halt_until_restart);
  return UNITY_END();
}
