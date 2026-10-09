// Native tests for peekWispBroadcast, which routes the wisp's status and
// config broadcasts around the single inbound control-op slot.

#include <unity.h>

#include <cstring>

#include "core/wisp_broadcast_peek.hpp"

using lamp::peekWispBroadcast;
using lamp::WispBroadcast;

namespace {

WispBroadcast peek(const char* s) {
  return peekWispBroadcast(reinterpret_cast<const uint8_t*>(s),
                           std::strlen(s));
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_status_and_config_classified() {
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::Status,
                        (int)peek(R"({"char":"wispStatus","source":"off"})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::Config,
                        (int)peek(R"({"char":"wispConfig","px":30})"));
}

// Everything else keeps the general inbound path.
void test_other_ops_pass_through() {
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None,
                        (int)peek(R"({"char":"brightness","value":5})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None,
                        (int)peek(R"({"char":"wispOp","op":"pollConfig"})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None,
                        (int)peek(R"({"source":"off","char":"wispStatus"})"));
}

// A truncated prefix is not a match and never reads past len.
void test_short_payload_is_none() {
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None, (int)peek(R"({"char":"wisp)"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None,
                        (int)peekWispBroadcast(nullptr, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_status_and_config_classified);
  RUN_TEST(test_other_ops_pass_through);
  RUN_TEST(test_short_payload_is_none);
  return UNITY_END();
}
