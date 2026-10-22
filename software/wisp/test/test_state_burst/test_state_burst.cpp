// Native tests for PresenceBeacon's STATE timing helpers.
//
// stateBurstDue gates the extra STATE copies emitted after a paint-mode edge:
//   - no window (burstUntilMs == 0) is never due,
//   - inside the window a copy is due only after intervalMs since the last emit,
//   - a closed window (now past burstUntilMs) is never due.
// stateTickOffsetMs maps a random word to the tick STATE's delay after the
// HELLO, covering [kStateOffsetMinMs, kStateOffsetMaxMs] evenly.
// TickStateSchedule holds the one tick STATE pending: due from its time until
// sent, never early, and a reschedule replaces an unsent one.
// nextStateSend picks at most one STATE per loop pass and none on a pass that
// already sent another fat frame. Its inputs come from PresenceBeacon::pump and
// StatusEmitter::pump, which stay out of the native build (Arduino WiFi, FreeRTOS
// timers, esp_random and the ESP-NOW MeshLink), so that wiring has no native
// test.

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
  constexpr uint32_t kPerBucket = 200;
  std::array<uint32_t, kOffsetSpan> hits{};
  uint32_t word = 0x2545F491u;
  for (uint32_t i = 0; i < kPerBucket * kOffsetSpan; ++i) {
    word = word * 1664525u + 1013904223u;
    const uint32_t off = wisp::stateTickOffsetMs(word);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(wisp::kStateOffsetMinMs, off);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(wisp::kStateOffsetMaxMs, off);
    ++hits[off - wisp::kStateOffsetMinMs];
  }
  for (uint32_t h : hits) {
    TEST_ASSERT_GREATER_THAN_UINT32(kPerBucket * 7 / 10, h);
    TEST_ASSERT_LESS_THAN_UINT32(kPerBucket * 13 / 10, h);
  }
}

void test_tick_state_not_due_before_scheduled(void) {
  wisp::TickStateSchedule s;
  TEST_ASSERT_FALSE(s.due(0));
  TEST_ASSERT_FALSE(s.due(123456));
}

void test_tick_state_due_from_offset_until_sent(void) {
  wisp::TickStateSchedule s;
  s.schedule(10000, 437);
  TEST_ASSERT_FALSE(s.due(10000));
  TEST_ASSERT_FALSE(s.due(10436));
  TEST_ASSERT_TRUE(s.due(10437));
  TEST_ASSERT_TRUE(s.due(11650));
  s.markSent();
  TEST_ASSERT_FALSE(s.due(11655));
  TEST_ASSERT_FALSE(s.due(11900));
}

void test_tick_state_reschedule_replaces_unsent(void) {
  wisp::TickStateSchedule s;
  s.schedule(10000, 700);
  s.schedule(12000, 250);
  TEST_ASSERT_FALSE(s.due(12249));
  TEST_ASSERT_TRUE(s.due(12250));
  s.markSent();
  TEST_ASSERT_FALSE(s.due(12900));
}

void test_tick_state_due_across_millis_wrap(void) {
  wisp::TickStateSchedule s;
  s.schedule(0xFFFFFF00u, 437);
  TEST_ASSERT_FALSE(s.due(0xFFFFFFF0u));
  TEST_ASSERT_FALSE(s.due(0x000000B4u));
  TEST_ASSERT_TRUE(s.due(0x000000B5u));
}

using wisp::StateSend;

void test_busy_pass_sends_no_state(void) {
  for (int m = 0; m < 8; ++m) {
    TEST_ASSERT_TRUE(wisp::nextStateSend(true, m & 1, m & 2, m & 4) ==
                     StateSend::None);
  }
}

void test_free_pass_sends_one_state_by_priority(void) {
  TEST_ASSERT_TRUE(wisp::nextStateSend(false, false, false, false) ==
                   StateSend::None);
  TEST_ASSERT_TRUE(wisp::nextStateSend(false, false, false, true) ==
                   StateSend::Tick);
  TEST_ASSERT_TRUE(wisp::nextStateSend(false, false, true, true) ==
                   StateSend::Burst);
  TEST_ASSERT_TRUE(wisp::nextStateSend(false, false, true, false) ==
                   StateSend::Burst);
  TEST_ASSERT_TRUE(wisp::nextStateSend(false, true, true, true) ==
                   StateSend::Edge);
  TEST_ASSERT_TRUE(wisp::nextStateSend(false, true, false, false) ==
                   StateSend::Edge);
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
  RUN_TEST(test_tick_state_due_from_offset_until_sent);
  RUN_TEST(test_tick_state_reschedule_replaces_unsent);
  RUN_TEST(test_tick_state_due_across_millis_wrap);
  RUN_TEST(test_busy_pass_sends_no_state);
  RUN_TEST(test_free_pass_sends_one_state_by_priority);
  return UNITY_END();
}
