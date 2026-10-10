// Native-host tests for webappShouldTeardown: the pure boot-window expiry
// decision, isolated from Arduino/WiFi so millis() wrap-safety is testable.

#include <unity.h>

#include <cstdint>

#include "../../src/components/webapp/webapp_deadline.hpp"

using namespace webapp;

void setUp(void) {}
void tearDown(void) {}

void test_never_expire_always_false() {
  TEST_ASSERT_FALSE(webappShouldTeardown(0, 1000, true));
  TEST_ASSERT_FALSE(webappShouldTeardown(5, UINT32_MAX, true));
  // Just after a millis() wrap, deadline still far ahead: must not tear down.
  TEST_ASSERT_FALSE(webappShouldTeardown(10, 0xFFFF0000UL, true));
}

void test_finite_deadline_before_and_after() {
  TEST_ASSERT_FALSE(webappShouldTeardown(999, 1000, false));
  TEST_ASSERT_TRUE(webappShouldTeardown(1000, 1000, false));
  TEST_ASSERT_TRUE(webappShouldTeardown(2000, 1000, false));
}

void test_wrap_safe_finite() {
  // Deadline was computed pre-wrap (0xFFFFFF00 + 0x200 wraps to 0x100).
  const uint32_t deadline = static_cast<uint32_t>(0xFFFFFF00UL + 0x200UL);
  // now past the wrapped deadline: elapsed, tear down.
  TEST_ASSERT_TRUE(webappShouldTeardown(0x00000200UL, deadline, false));
  // now still before it (pre-wrap side): not yet.
  TEST_ASSERT_FALSE(webappShouldTeardown(0xFFFFFF80UL, deadline, false));
}

void test_restore_ble_reboots_once_idle_regardless_of_station() {
  // BLE down + idle window elapsed -> restore (reboot), even with a station
  // still on.
  TEST_ASSERT_TRUE(webappShouldRestoreBle(/*now=*/200000, /*deadlineMs=*/100000,
                                          /*bleDown=*/true));
  // fresh activity (deadline in future) -> hold BLE down.
  TEST_ASSERT_FALSE(webappShouldRestoreBle(150000, 300000, true));
  // BLE not down -> this path never fires.
  TEST_ASSERT_FALSE(webappShouldRestoreBle(200000, 100000, false));
}

// A web save's reboot fires once due; only an OTA in progress holds it.
void test_armed_reboot_waits_for_ota_only() {
  TEST_ASSERT_FALSE(webapp::webappRebootDue(1000, 0, false));
  TEST_ASSERT_FALSE(webapp::webappRebootDue(999, 1000, false));
  TEST_ASSERT_TRUE(webapp::webappRebootDue(1000, 1000, false));
  TEST_ASSERT_FALSE(webapp::webappRebootDue(5000, 1000, true));
  TEST_ASSERT_TRUE(webapp::webappRebootDue(0x00000010u, 0xFFFFFFF0u, false));
}

// A stuck OTA holds the reboot only kRebootOtaWaitMaxMs past its time.
void test_armed_reboot_waits_out_ota_up_to_the_ceiling() {
  const uint32_t at = 1000;
  TEST_ASSERT_FALSE(webapp::webappRebootDue(at + webapp::kRebootOtaWaitMaxMs - 1, at, true));
  TEST_ASSERT_TRUE(webapp::webappRebootDue(at + webapp::kRebootOtaWaitMaxMs, at, true));
  TEST_ASSERT_TRUE(webapp::webappRebootDue(0x00000010u + webapp::kRebootOtaWaitMaxMs,
                                           0x00000010u, true));
  TEST_ASSERT_FALSE(webapp::webappRebootDue(at + webapp::kRebootOtaWaitMaxMs, 0, true));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_never_expire_always_false);
  RUN_TEST(test_finite_deadline_before_and_after);
  RUN_TEST(test_wrap_safe_finite);
  RUN_TEST(test_restore_ble_reboots_once_idle_regardless_of_station);
  RUN_TEST(test_armed_reboot_waits_for_ota_only);
  RUN_TEST(test_armed_reboot_waits_out_ota_up_to_the_ceiling);
  return UNITY_END();
}
