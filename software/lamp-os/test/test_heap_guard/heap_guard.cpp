// Native tests for the one-shot allocation guard every NVS/JSON save goes
// through: refuse rather than allocate into the last free block, and never
// hand back a document that was truncated while it was built.

#include <unity.h>

#include <cstdlib>
#include <string>

#include "util/heap_guard.hpp"

#include "../../src/util/heap_guard.cpp"

using namespace lamp;

void setUp(void) { g_mock_largest_free_block = 1 << 20; }
void tearDown(void) {}

void test_spare_needs_headroom_past_the_request() {
  g_mock_largest_free_block = 1000 + kHeapHeadroom;
  TEST_ASSERT_TRUE(heapCanSpare(1000));
  TEST_ASSERT_FALSE(heapCanSpare(1001));
  TEST_ASSERT_NULL(allocateSpare(1001).get());
  TEST_ASSERT_NOT_NULL(allocateSpare(1000).get());
}

void test_serialize_matches_serializejson() {
  JsonDocument doc;
  doc["lamp"]["name"] = "jacko";
  doc["expressions"].add(42);
  std::string expected;
  serializeJson(doc, expected);
  size_t len = 0;
  auto buf = serializeGuarded(doc, len);
  TEST_ASSERT_NOT_NULL(buf.get());
  TEST_ASSERT_EQUAL(expected.size(), len);
  TEST_ASSERT_EQUAL_STRING(expected.c_str(), buf.get());
}

void test_serialize_refuses_on_low_heap() {
  JsonDocument doc;
  doc["k"] = "v";
  g_mock_largest_free_block = kHeapHeadroom;
  size_t len = 99;
  TEST_ASSERT_NULL(serializeGuarded(doc, len).get());
  TEST_ASSERT_EQUAL(0, len);
}

// Fails every allocation once armed, the way a fragmented heap fails
// ArduinoJson's pool growth partway through a document.
struct FailingAllocator : ArduinoJson::Allocator {
  bool fail = false;
  void* allocate(size_t n) override { return fail ? nullptr : std::malloc(n); }
  void deallocate(void* p) override { std::free(p); }
  void* reallocate(void* p, size_t n) override {
    return fail ? nullptr : std::realloc(p, n);
  }
};

void test_serialize_refuses_an_overflowed_document() {
  FailingAllocator alloc;
  JsonDocument doc(&alloc);
  doc["lamp"]["name"] = "jacko";
  alloc.fail = true;
  for (int i = 0; i < 200; ++i) doc["expressions"].add(std::string(40, 'x'));
  TEST_ASSERT_TRUE(doc.overflowed());
  size_t len = 0;
  TEST_ASSERT_NULL(serializeGuarded(doc, len).get());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_spare_needs_headroom_past_the_request);
  RUN_TEST(test_serialize_matches_serializejson);
  RUN_TEST(test_serialize_refuses_on_low_heap);
  RUN_TEST(test_serialize_refuses_an_overflowed_document);
  return UNITY_END();
}
