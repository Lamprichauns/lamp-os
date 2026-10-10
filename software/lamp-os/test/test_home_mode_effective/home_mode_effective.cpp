// Native truth-table tests for effectiveHomeModeFromConfig.
// Includes only home_mode_logic.hpp — no BLE/WiFi/Arduino deps.

#include <unity.h>

#include "core/home_mode_logic.hpp"

using namespace lamp;

void setUp(void) {}
void tearDown(void) {}

void test_disabled_always_false() {
  TEST_ASSERT_FALSE(effectiveHomeModeFromConfig(false, false, true, false));
  TEST_ASSERT_FALSE(effectiveHomeModeFromConfig(false, true, false, true));
}

void test_manual_mode_always_true_when_enabled() {
  // networkBound=false: home mode is on regardless of SSID state.
  TEST_ASSERT_TRUE(effectiveHomeModeFromConfig(true, false, true, false));
  TEST_ASSERT_TRUE(effectiveHomeModeFromConfig(true, false, false, false));
  TEST_ASSERT_TRUE(effectiveHomeModeFromConfig(true, false, false, true));
}

void test_network_bound_requires_ssid_and_visible() {
  TEST_ASSERT_FALSE(effectiveHomeModeFromConfig(true, true, true, false));   // no ssid
  TEST_ASSERT_FALSE(effectiveHomeModeFromConfig(true, true, false, false));  // ssid set, not visible
  TEST_ASSERT_TRUE(effectiveHomeModeFromConfig(true, true, false, true));    // ssid + visible
}

// One snapshot decides both: a connected app turns home mode on only from its
// Home Mode page, and that is a preview; without one, config decides and
// nothing is a preview.
void test_state_from_one_client_snapshot() {
  HomeModeState s = homeModeState(true, true, false);
  TEST_ASSERT_TRUE(s.active);
  TEST_ASSERT_TRUE(s.preview);

  s = homeModeState(true, false, true);  // config says home, app overrides
  TEST_ASSERT_FALSE(s.active);
  TEST_ASSERT_FALSE(s.preview);

  s = homeModeState(false, true, true);  // page flag left over, no client
  TEST_ASSERT_TRUE(s.active);
  TEST_ASSERT_FALSE(s.preview);

  s = homeModeState(false, false, false);
  TEST_ASSERT_FALSE(s.active);
  TEST_ASSERT_FALSE(s.preview);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_disabled_always_false);
  RUN_TEST(test_manual_mode_always_true_when_enabled);
  RUN_TEST(test_network_bound_requires_ssid_and_visible);
  RUN_TEST(test_state_from_one_client_snapshot);
  return UNITY_END();
}
