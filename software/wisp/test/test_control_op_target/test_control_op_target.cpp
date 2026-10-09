// Native tests for the CONTROL_OP target filter in MeshRouter. onPacket is
// Arduino/dispatcher-coupled and not natively buildable; controlOpForWisp is
// the pure predicate it gates on, so this pins the single-hop scope contract:
//   - broadcast (all-0xFF) applies at any wisp.
//   - a target matching this wisp's MAC applies.
//   - a target for a sibling wisp is dropped.
//   - a relay of this wisp's own broadcast is dropped.

#include <unity.h>

#include <cstdint>

#include "net/mesh_router.hpp"

namespace {

const uint8_t kSelf[6]      = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
const uint8_t kSibling[6]   = {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0};
const uint8_t kBroadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t kApp[6]       = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

void test_broadcast_applies(void) {
  TEST_ASSERT_TRUE(wisp::controlOpForWisp(kBroadcast, kApp, kSelf));
}

void test_addressed_to_self_applies(void) {
  TEST_ASSERT_TRUE(wisp::controlOpForWisp(kSelf, kApp, kSelf));
}

void test_addressed_to_sibling_drops(void) {
  TEST_ASSERT_FALSE(wisp::controlOpForWisp(kSibling, kApp, kSelf));
}

// Pre-setSelfMac (myMac all-zero) still applies broadcast, drops any unicast.
void test_unset_self_mac_accepts_only_broadcast(void) {
  const uint8_t zero[6] = {0, 0, 0, 0, 0, 0};
  TEST_ASSERT_TRUE(wisp::controlOpForWisp(kBroadcast, kApp, zero));
  TEST_ASSERT_FALSE(wisp::controlOpForWisp(kSelf, kApp, zero));
}

// A lamp relaying this wisp's own wispStatus / wispConfig broadcast back to
// it must not reach the single op slot.
void test_own_broadcast_echo_drops(void) {
  TEST_ASSERT_FALSE(wisp::controlOpForWisp(kBroadcast, kSelf, kSelf));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_broadcast_applies);
  RUN_TEST(test_addressed_to_self_applies);
  RUN_TEST(test_addressed_to_sibling_drops);
  RUN_TEST(test_unset_self_mac_accepts_only_broadcast);
  RUN_TEST(test_own_broadcast_echo_drops);
  return UNITY_END();
}
