// Native tests for when Config's saves run: backoff after a failure, a quiet
// hold once a web document is stored, and a write-back after a web document's
// save fails partway.

#include <unity.h>

#include "config/save_gate.hpp"

using lamp::SaveGate;
using lamp::config_codec::SaveResult;

void setUp(void) {}
void tearDown(void) {}

void test_a_failed_save_backs_off_until_one_lands() {
  SaveGate gate(5000, 60000);
  TEST_ASSERT_TRUE(gate.allows());
  TEST_ASSERT_TRUE(gate.due(0));
  gate.failed(1000);
  TEST_ASSERT_TRUE(gate.pending());
  TEST_ASSERT_FALSE(gate.due(5999));
  TEST_ASSERT_TRUE(gate.due(6000));
  gate.saved();
  TEST_ASSERT_FALSE(gate.pending());
}

// The commit drain asks due() every loop and persistConfig asks allows(); a
// stored web document says no to both, with nothing pending, until the reboot.
// A second web document is held too, so it cannot write over the first.
void test_a_stored_web_document_holds_every_save() {
  SaveGate gate(5000, 60000);
  gate.failed(1000);
  gate.rawFinished(SaveResult::Ok, 2000);
  TEST_ASSERT_FALSE(gate.allows());
  TEST_ASSERT_FALSE(gate.pending());
  for (uint32_t now = 0; now < 200000; now += 1000) TEST_ASSERT_FALSE(gate.due(now));
  gate.failed(3000);
  TEST_ASSERT_FALSE(gate.pending());
}

// Only a save that wrote partway leaves the running config to write back;
// one refused before any write leaves nothing to repair.
void test_a_failed_web_document_save_writes_the_running_config_back() {
  for (SaveResult r : {SaveResult::Invalid, SaveResult::NoHeap, SaveResult::TooLarge}) {
    SaveGate gate(5000, 60000);
    gate.rawFinished(r, 1000);
    TEST_ASSERT_TRUE(gate.allows());
    TEST_ASSERT_FALSE(gate.pending());
  }
  SaveGate gate(5000, 60000);
  gate.rawFinished(SaveResult::WriteFailed, 1000);
  TEST_ASSERT_TRUE(gate.allows());
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
