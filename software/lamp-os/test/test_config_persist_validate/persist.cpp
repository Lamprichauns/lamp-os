// Native tests for saving a whole web document (config_codec::saveRawJson,
// which Config::persistRawJson runs): a truncated or lamp-less body is
// refused before any write, a heap that cannot spare the parse refuses before
// any allocation, and a good document saves in the stored shape.

#include <unity.h>

#include <ArduinoJson.h>

#include <cstring>

#include "config/config_codec.hpp"
#include "config/config_store.hpp"
#include "esp_heap_caps.h"

// Native tests don't build src/, so compile the real implementations in.
#include "../../src/util/color.cpp"
#include "../../src/config/config_codec.cpp"
#include "../../src/util/heap_guard.cpp"
#include "../../src/util/json_writer.cpp"

using namespace lamp;
using config_codec::SaveResult;

void setUp(void) { g_mock_largest_free_block = 1 << 20; }
void tearDown(void) {}

static char s_buf[config_codec::kMaxStoredCfgJsonLen + 1];

static SaveResult persist(InMemoryConfigStore& s, const char* json) {
  return config_codec::saveRawJson(s, json, s_buf, sizeof(s_buf));
}

static std::string storedName(InMemoryConfigStore& s) {
  JsonDocument doc;
  deserializeJson(doc, s.read("cfg", "{}"));
  return doc["lamp"]["name"] | "";
}

static const char* kGood = R"({"lamp":{"name":"jacko","named":true}})";
static const char* kTruncated = R"({"lamp":{"name":"jac)";  // silent-OOM shape
static const char* kNoLampObject = R"({"base":{"px":32}})";

void test_truncated_blob_rejected_and_store_untouched() {
  InMemoryConfigStore s;
  TEST_ASSERT_TRUE(persist(s, kGood) == SaveResult::Ok);
  const size_t writes = s.writeCount;
  TEST_ASSERT_TRUE(persist(s, kTruncated) == SaveResult::Invalid);
  TEST_ASSERT_EQUAL(writes, s.writeCount);
  TEST_ASSERT_EQUAL_STRING("jacko", storedName(s).c_str());
}

void test_blob_without_lamp_object_rejected() {
  InMemoryConfigStore s;
  TEST_ASSERT_TRUE(persist(s, kNoLampObject) == SaveResult::Invalid);
  TEST_ASSERT_EQUAL(0, s.writeCount);
}

void test_valid_whole_document_saved_in_the_stored_shape() {
  InMemoryConfigStore s;
  TEST_ASSERT_TRUE(persist(s, kGood) == SaveResult::Ok);
  TEST_ASSERT_EQUAL_STRING("jacko", storedName(s).c_str());
  TEST_ASSERT_TRUE(s.read("cfg", "").find("\"expressions\":[]") != std::string::npos);
}

void test_null_rejected() {
  InMemoryConfigStore s;
  TEST_ASSERT_TRUE(persist(s, nullptr) == SaveResult::Invalid);
}

// The parse and the model need about three times the document plus the
// model's ceiling; a heap that cannot spare it refuses before allocating.
void test_low_heap_refuses_before_parsing() {
  InMemoryConfigStore s;
  const size_t need = config_codec::kRawSaveHeapPerByte * std::strlen(kGood) +
                      config_codec::kRawSaveModelBytes + kHeapHeadroom;
  g_mock_largest_free_block = need - 1;
  TEST_ASSERT_TRUE(persist(s, kGood) == SaveResult::NoHeap);
  TEST_ASSERT_EQUAL(0, s.writeCount);
  g_mock_largest_free_block = need;
  TEST_ASSERT_TRUE(persist(s, kGood) == SaveResult::Ok);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_truncated_blob_rejected_and_store_untouched);
  RUN_TEST(test_blob_without_lamp_object_rejected);
  RUN_TEST(test_valid_whole_document_saved_in_the_stored_shape);
  RUN_TEST(test_null_rejected);
  RUN_TEST(test_low_heap_refuses_before_parsing);
  return UNITY_END();
}
