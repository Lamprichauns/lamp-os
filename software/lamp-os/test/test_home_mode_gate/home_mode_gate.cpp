// Native tests for the compositor home-mode skip predicate.
// Tests homeModeExpressionSkips from home_mode_gate.hpp directly —
// no Compositor, AnimatedBehavior, or Arduino deps required.

#include <unity.h>

#include <string>
#include <vector>

#include "core/home_mode_gate.hpp"

using namespace lamp;

void setUp(void) {}
void tearDown(void) {}

void test_null_id_not_skipped() {
  TEST_ASSERT_FALSE(homeModeExpressionSkips(nullptr, {"glitchy"}));
}

void test_expression_in_list_skipped() {
  TEST_ASSERT_TRUE(homeModeExpressionSkips("glitchy", {"glitchy"}));
}

void test_expression_not_in_list_not_skipped() {
  TEST_ASSERT_FALSE(homeModeExpressionSkips("shifty", {"glitchy"}));
}

void test_empty_disabled_list_skips_nothing() {
  TEST_ASSERT_FALSE(homeModeExpressionSkips("glitchy", {}));
}

void test_null_id_with_empty_list_not_skipped() {
  TEST_ASSERT_FALSE(homeModeExpressionSkips(nullptr, {}));
}

void test_multiple_disabled_expressions() {
  std::vector<std::string> disabled = {"glitchy", "pulse"};
  TEST_ASSERT_TRUE(homeModeExpressionSkips("glitchy", disabled));
  TEST_ASSERT_TRUE(homeModeExpressionSkips("pulse", disabled));
  TEST_ASSERT_FALSE(homeModeExpressionSkips("shifty", disabled));
  TEST_ASSERT_FALSE(homeModeExpressionSkips("breathing", disabled));
}

void test_case_sensitive_match() {
  TEST_ASSERT_FALSE(homeModeExpressionSkips("Glitchy", {"glitchy"}));
  TEST_ASSERT_FALSE(homeModeExpressionSkips("GLITCHY", {"glitchy"}));
}

// Mirror of Compositor::homeModeSkipsType: the received-cascade drop decision.
// A cascade is dropped at receipt only when home mode is ON and the type is
// in the disabled set; otherwise it creates + renders as normal.
static bool receivedCascadeDropped(bool homeMode, const char* type,
                                   const std::vector<std::string>& disabled) {
  return homeMode && homeModeExpressionSkips(type, disabled);
}

void test_received_cascade_home_off_never_dropped() {
  TEST_ASSERT_FALSE(receivedCascadeDropped(false, "glitchy", {"glitchy"}));
  TEST_ASSERT_FALSE(receivedCascadeDropped(false, "shifty", {"glitchy"}));
}

void test_received_cascade_home_on_enabled_type_kept() {
  TEST_ASSERT_FALSE(receivedCascadeDropped(true, "shifty", {"glitchy"}));
}

void test_received_cascade_home_on_disabled_type_dropped() {
  TEST_ASSERT_TRUE(receivedCascadeDropped(true, "glitchy", {"glitchy"}));
}

// Peer pushes are ignored only when home mode is active AND social is off.
void test_remote_blocked_only_home_and_social_off() {
  TEST_ASSERT_TRUE(homeModeBlocksRemote(true, true, TransientSource::Remote));
  TEST_ASSERT_FALSE(homeModeBlocksRemote(true, false, TransientSource::Remote));
  TEST_ASSERT_FALSE(homeModeBlocksRemote(false, true, TransientSource::Remote));
  TEST_ASSERT_FALSE(homeModeBlocksRemote(false, false, TransientSource::Remote));
}

void test_local_never_blocked() {
  TEST_ASSERT_FALSE(homeModeBlocksRemote(true, true, TransientSource::Local));
  TEST_ASSERT_FALSE(homeModeBlocksRemote(true, false, TransientSource::Local));
  TEST_ASSERT_FALSE(homeModeBlocksRemote(false, true, TransientSource::Local));
  TEST_ASSERT_FALSE(homeModeBlocksRemote(false, false, TransientSource::Local));
}

// Social off skips a social behavior unless it plays the app's own greeting.
void test_social_skip_lets_app_greeting_play() {
  TEST_ASSERT_TRUE(homeModeSkipsSocial(true, false));
  TEST_ASSERT_FALSE(homeModeSkipsSocial(true, true));
  TEST_ASSERT_FALSE(homeModeSkipsSocial(false, false));
  TEST_ASSERT_FALSE(homeModeSkipsSocial(false, true));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_null_id_not_skipped);
  RUN_TEST(test_expression_in_list_skipped);
  RUN_TEST(test_expression_not_in_list_not_skipped);
  RUN_TEST(test_empty_disabled_list_skips_nothing);
  RUN_TEST(test_null_id_with_empty_list_not_skipped);
  RUN_TEST(test_multiple_disabled_expressions);
  RUN_TEST(test_case_sensitive_match);
  RUN_TEST(test_received_cascade_home_off_never_dropped);
  RUN_TEST(test_received_cascade_home_on_enabled_type_kept);
  RUN_TEST(test_received_cascade_home_on_disabled_type_dropped);
  RUN_TEST(test_remote_blocked_only_home_and_social_off);
  RUN_TEST(test_local_never_blocked);
  RUN_TEST(test_social_skip_lets_app_greeting_play);
  return UNITY_END();
}
