// Native tests for the STATE edge-burst timing predicate in PresenceBeacon.
//
// stateBurstDue gates the extra STATE copies emitted after a paint-mode edge:
//   - no window (burstUntilMs == 0) is never due,
//   - inside the window a copy is due only after intervalMs since the last emit,
//   - a closed window (now past burstUntilMs) is never due.
// stateTickOffsetMs maps a random word to the tick STATE's delay after the
// HELLO, covering [kStateOffsetMinMs, kStateOffsetMaxMs] evenly.
// TickStateSchedule holds the one tick STATE pending: due once at or after its
// time, never early, never twice, and a reschedule replaces an unsent one.

#include <unity.h>

#include <array>
#include <cstdint>

#include "status/presence_beacon.hpp"

namespace {

constexpr uint32_t kInterval = 700;

void test_no_window_never_due(void) {
  TEST_ASSERT_FALSE(wisp::stateBurstDue(5000, 0, 4000, kInterval));
}

void test_open_window_after_interval_is_due(void) {
  TEST_ASSERT_TRUE(wisp::stateBurstDue(2000, 3000, 1300, kInterval));
}

void test_open_window_too_soon_not_due(void) {
  TEST_ASSERT_FALSE(wisp::stateBurstDue(2000, 3000, 1500, kInterval));
}

void test_interval_boundary_is_due(void) {
  TEST_ASSERT_TRUE(wisp::stateBurstDue(2000, 3000, 1300, kInterval));
  TEST_ASSERT_FALSE(wisp::stateBurstDue(1999, 3000, 1300, kInterval));
}

void test_closed_window_not_due(void) {
  TEST_ASSERT_FALSE(wisp::stateBurstDue(3000, 3000, 100, kInterval));
  TEST_ASSERT_FALSE(wisp::stateBurstDue(3500, 3000, 100, kInterval));
}

constexpr uint32_t kOffsetSpan = wisp::kStateOffsetMaxMs - wisp::kStateOffsetMinMs + 1;

void test_offset_stays_in_window(void) {
  const uint32_t words[] = {0u, 1u, 599u, 600u, 601u, 12345u, 0x7FFFFFFFu,
                            0x80000000u, 0xFFFFFFFEu, 0xFFFFFFFFu};
  for (uint32_t w : words) {
    const uint32_t off = wisp::stateTickOffsetMs(w);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(wisp::kStateOffsetMinMs, off);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(wisp::kStateOffsetMaxMs, off);
  }
}

void test_offset_covers_window_evenly(void) {
  std::array<uint32_t, kOffsetSpan> hits{};
  const uint32_t base = 0x9E3779B9u;
  for (uint32_t i = 0; i < 2 * kOffsetSpan; ++i) {
    const uint32_t off = wisp::stateTickOffsetMs(base + i);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(wisp::kStateOffsetMinMs, off);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(wisp::kStateOffsetMaxMs, off);
    ++hits[off - wisp::kStateOffsetMinMs];
  }
  for (uint32_t h : hits) TEST_ASSERT_EQUAL_UINT32(2, h);
}

void test_tick_state_not_due_before_scheduled(void) {
  wisp::TickStateSchedule s;
  TEST_ASSERT_FALSE(s.takeDue(0));
  TEST_ASSERT_FALSE(s.takeDue(123456));
}

void test_tick_state_due_once_at_offset(void) {
  wisp::TickStateSchedule s;
  s.schedule(10000, 437);
  TEST_ASSERT_FALSE(s.takeDue(10000));
  TEST_ASSERT_FALSE(s.takeDue(10436));
  TEST_ASSERT_TRUE(s.takeDue(10437));
  TEST_ASSERT_FALSE(s.takeDue(10437));
  TEST_ASSERT_FALSE(s.takeDue(11900));
}

void test_tick_state_late_pass_still_sends(void) {
  wisp::TickStateSchedule s;
  s.schedule(10000, 437);
  TEST_ASSERT_TRUE(s.takeDue(11650));
  TEST_ASSERT_FALSE(s.takeDue(11655));
}

void test_tick_state_reschedule_replaces_unsent(void) {
  wisp::TickStateSchedule s;
  s.schedule(10000, 700);
  s.schedule(12000, 250);
  TEST_ASSERT_FALSE(s.takeDue(12249));
  TEST_ASSERT_TRUE(s.takeDue(12250));
  TEST_ASSERT_FALSE(s.takeDue(12900));
}

void test_tick_state_due_across_millis_wrap(void) {
  wisp::TickStateSchedule s;
  s.schedule(0xFFFFFF00u, 437);
  TEST_ASSERT_FALSE(s.takeDue(0xFFFFFFF0u));
  TEST_ASSERT_FALSE(s.takeDue(0x000000B4u));
  TEST_ASSERT_TRUE(s.takeDue(0x000000B5u));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_no_window_never_due);
  RUN_TEST(test_open_window_after_interval_is_due);
  RUN_TEST(test_open_window_too_soon_not_due);
  RUN_TEST(test_interval_boundary_is_due);
  RUN_TEST(test_closed_window_not_due);
  RUN_TEST(test_offset_stays_in_window);
  RUN_TEST(test_offset_covers_window_evenly);
  RUN_TEST(test_tick_state_not_due_before_scheduled);
  RUN_TEST(test_tick_state_due_once_at_offset);
  RUN_TEST(test_tick_state_late_pass_still_sends);
  RUN_TEST(test_tick_state_reschedule_replaces_unsent);
  RUN_TEST(test_tick_state_due_across_millis_wrap);
  return UNITY_END();
}
