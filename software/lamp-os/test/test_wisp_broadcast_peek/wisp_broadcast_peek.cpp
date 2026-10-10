// Native tests for peekWispBroadcast, which routes the wisp's status and
// config broadcasts around the single inbound control-op slot.

#include <unity.h>

#include <cstring>
#include <initializer_list>

#include "core/wisp_broadcast_peek.hpp"

using lamp::peekWispBroadcast;
using lamp::wispBroadcastTag;
using lamp::inboundControlOpKind;
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

// The inbound drain takes only the wisp's status and config from the mesh.
void test_tag_takes_only_wisp_status_and_config() {
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::Status, (int)wispBroadcastTag("wispStatus"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::Config, (int)wispBroadcastTag("wispConfig"));
  for (const char* tag : {"brightness", "shadeColors", "baseColors", "knockout",
                          "expressionOp", "wifiOp", "wispOp", "wispStatusX", ""}) {
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)WispBroadcast::None, (int)wispBroadcastTag(tag), tag);
  }
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None, (int)wispBroadcastTag(nullptr));
}

// The inbound drain routes a wisp broadcast whose "char" is not first, and
// drops every lamp-setting op and anything unparseable.
void test_inbound_payload_routed_by_parsed_tag() {
  auto kind = [](const char* s) { return (int)inboundControlOpKind(s, std::strlen(s)); };
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::Status, kind(R"({"source":"off","char":"wispStatus"})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::Config, kind(R"({"px":30,"char":"wispConfig"})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None, kind(R"({"char":"brightness","value":5})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None, kind(R"({"char":"expressionOp","op":"upsert"})"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None, kind(R"({"char":"wispStatus")"));
  TEST_ASSERT_EQUAL_INT((int)WispBroadcast::None, kind(R"({"value":5})"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_status_and_config_classified);
  RUN_TEST(test_other_ops_pass_through);
  RUN_TEST(test_short_payload_is_none);
  RUN_TEST(test_tag_takes_only_wisp_status_and_config);
  RUN_TEST(test_inbound_payload_routed_by_parsed_tag);
  return UNITY_END();
}
