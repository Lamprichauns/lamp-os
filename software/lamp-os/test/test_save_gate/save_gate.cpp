// Native tests for when Config's saves run: backoff after a failure, a quiet
// hold once a web document is stored, and a write-back after a web document's
// save fails.

#include <unity.h>

#include "config/save_gate.hpp"

using lamp::SaveGate;

void setUp(void) {}
void tearDown(void) {}

void test_a_failed_save_backs_off_until_one_lands() {
  SaveGate gate(5000, 60000);
  TEST_ASSERT_TRUE(gate.due(0));
  gate.failed(1000);
  TEST_ASSERT_TRUE(gate.pending());
  TEST_ASSERT_FALSE(gate.due(5999));
  TEST_ASSERT_TRUE(gate.due(6000));
  gate.saved();
  TEST_ASSERT_FALSE(gate.pending());
}

// The commit drain asks due() every loop; a frozen gate says no, so the drain
// neither saves nor logs until the reboot.
void test_a_stored_web_document_holds_every_save() {
  SaveGate gate(5000, 60000);
  gate.failed(1000);
  gate.rawSaved();
  TEST_ASSERT_TRUE(gate.frozen());
  TEST_ASSERT_FALSE(gate.pending());
  for (uint32_t now = 0; now < 200000; now += 1000) TEST_ASSERT_FALSE(gate.due(now));
}

void test_a_failed_web_document_save_writes_the_running_config_back() {
  SaveGate gate(5000, 60000);
  gate.rawFailed(1000);
  TEST_ASSERT_FALSE(gate.frozen());
  TEST_ASSERT_TRUE(gate.pending());
  TEST_ASSERT_TRUE(gate.due(6000));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_a_failed_save_backs_off_until_one_lands);
  RUN_TEST(test_a_stored_web_document_holds_every_save);
  RUN_TEST(test_a_failed_web_document_save_writes_the_running_config_back);
  return UNITY_END();
}
