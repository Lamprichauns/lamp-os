// Native-host tests for pskUsable: the WPA2 passphrase length window that
// decides whether the softAP comes up keyed or open.
#include <unity.h>

#include "../../src/components/network/transport/wifi_psk.hpp"

using wifi::pskUsable;

void setUp(void) {}
void tearDown(void) {}

void test_no_password_is_not_usable() {
  TEST_ASSERT_FALSE(pskUsable(0));
}

void test_below_wpa2_minimum_is_not_usable() {
  TEST_ASSERT_FALSE(pskUsable(1));
  TEST_ASSERT_FALSE(pskUsable(7));
}

void test_wpa2_window_is_usable() {
  TEST_ASSERT_TRUE(pskUsable(8));
  TEST_ASSERT_TRUE(pskUsable(16));
  TEST_ASSERT_TRUE(pskUsable(63));
}

void test_above_wpa2_maximum_is_not_usable() {
  TEST_ASSERT_FALSE(pskUsable(64));
  TEST_ASSERT_FALSE(pskUsable(256));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_no_password_is_not_usable);
  RUN_TEST(test_below_wpa2_minimum_is_not_usable);
  RUN_TEST(test_wpa2_window_is_usable);
  RUN_TEST(test_above_wpa2_maximum_is_not_usable);
  return UNITY_END();
}
