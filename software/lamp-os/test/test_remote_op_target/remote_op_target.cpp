// Native pin for the CHAR_REMOTE_OP target decision: applied on this lamp for
// its own MAC or "broadcast", never for another lamp, a missing or a malformed
// target.

#include <unity.h>

#include "core/remote_op_target.hpp"

using lamp::remoteOpTargetsLamp;

namespace {
const uint8_t kMine[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
}

void setUp() {}
void tearDown() {}

void test_self_applies() {
  TEST_ASSERT_TRUE(remoteOpTargetsLamp("AA:BB:CC:DD:EE:01", kMine));
}

void test_broadcast_applies() {
  TEST_ASSERT_TRUE(remoteOpTargetsLamp("broadcast", kMine));
}

void test_other_lamp_ignored() {
  TEST_ASSERT_FALSE(remoteOpTargetsLamp("AA:BB:CC:DD:EE:02", kMine));
}

void test_missing_or_malformed_ignored() {
  TEST_ASSERT_FALSE(remoteOpTargetsLamp(nullptr, kMine));
  TEST_ASSERT_FALSE(remoteOpTargetsLamp("", kMine));
  TEST_ASSERT_FALSE(remoteOpTargetsLamp("self", kMine));
  TEST_ASSERT_FALSE(remoteOpTargetsLamp("AA:BB:CC:DD:EE", kMine));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_self_applies);
  RUN_TEST(test_broadcast_applies);
  RUN_TEST(test_other_lamp_ignored);
  RUN_TEST(test_missing_or_malformed_ignored);
  return UNITY_END();
}
