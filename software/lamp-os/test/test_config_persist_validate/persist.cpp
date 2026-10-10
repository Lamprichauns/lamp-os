// Native tests for saving a whole web document (config_codec::saveRawJson,
// which Config::persistRawJson runs): a truncated or lamp-less body is
// refused before any write, a heap that cannot spare the parse refuses before
// any allocation, and a good document saves in the stored shape.

#include <unity.h>

#include <ArduinoJson.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <string>

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

// Heap use while a web document is saved, counted on the host: the parse
// through a counting ArduinoJson allocator, the model through operator new.
// Host slots and pools are larger than the ESP32's (64-bit pointers), so the
// figures run high.
namespace {

struct Tally {
  size_t live = 0;
  size_t peak = 0;
  size_t largest = 0;
  void add(size_t n) {
    live += n;
    peak = std::max(peak, live);
    largest = std::max(largest, n);
  }
};

Tally g_newTally;
bool g_counting = false;

class CountingAllocator : public ArduinoJson::Allocator {
 public:
  Tally tally;
  void* allocate(size_t size) override {
    void* p = std::malloc(size);
    if (p) {
      sizes_[p] = size;
      tally.add(size);
    }
    return p;
  }
  void deallocate(void* p) override {
    tally.live -= sizes_[p];
    sizes_.erase(p);
    std::free(p);
  }
  void* reallocate(void* p, size_t size) override {
    // A realloc holds both blocks at its peak.
    void* q = allocate(size);
    if (q && p) {
      std::memcpy(q, p, std::min(size, sizes_[p]));
      deallocate(p);
    }
    return q;
  }

 private:
  std::map<void*, size_t> sizes_;
};

}  // namespace

void* operator new(size_t size) {
  void* p = std::malloc(size + sizeof(size_t));
  if (!p) throw std::bad_alloc();
  *static_cast<size_t*>(p) = size;
  if (g_counting) g_newTally.add(size);
  return static_cast<size_t*>(p) + 1;
}
void operator delete(void* p) noexcept {
  if (!p) return;
  size_t* base = static_cast<size_t*>(p) - 1;
  if (g_counting) g_newTally.live -= std::min(g_newTally.live, *base);
  std::free(base);
}
void operator delete(void* p, size_t) noexcept { operator delete(p); }

void setUp(void) {
  g_mock_largest_free_block = 1 << 20;
  g_mock_free_size = 1 << 20;
}
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
// Short of either figure the gate takes, the save is refused before any
// allocation; with both it goes ahead.
void test_low_heap_refuses_before_parsing() {
  const auto shape = config_codec::scanRawJson(kGood);
  for (int short_ : {0, 1}) {
    InMemoryConfigStore s;
    g_mock_free_size = shape.heapBytes() + kHeapHeadroom - (short_ == 0 ? 1 : 0);
    g_mock_largest_free_block = shape.largestBlock() + kHeapHeadroom - (short_ == 1 ? 1 : 0);
    TEST_ASSERT_TRUE(persist(s, kGood) == SaveResult::NoHeap);
    TEST_ASSERT_EQUAL(0, s.writeCount);
  }
  InMemoryConfigStore s;
  g_mock_free_size = shape.heapBytes() + kHeapHeadroom;
  g_mock_largest_free_block = shape.largestBlock() + kHeapHeadroom;
  TEST_ASSERT_TRUE(persist(s, kGood) == SaveResult::Ok);
}

namespace {

struct Measured {
  size_t len;
  size_t peak;     // the parse's peak plus the model's, both alive together
  size_t largest;  // the largest single block either takes
};

Measured measure(const std::string& json) {
  CountingAllocator parse;
  JsonDocument doc(&parse);
  TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
  g_newTally = {};
  g_counting = true;
  {
    LampSettings lamp;
    BaseSettings base;
    ShadeSettings shade;
    ExpressionSettings expressions;
    HomeModeSettings homeMode;
    config_codec::fromJson(doc.as<JsonObject>(), lamp, base, shade, expressions, homeMode);
  }
  g_counting = false;
  return {json.size(), parse.tally.peak + g_newTally.peak,
          std::max(parse.tally.largest, g_newTally.largest)};
}

ExpressionConfig full(const char* type, uint8_t target) {
  ExpressionConfig e;
  e.type = type;
  e.target = target;
  for (size_t i = 0; i < kMaxExpressionColors; ++i) e.colors.push_back(Color(1, 2, 3, 4));
  for (size_t i = 0; i < kMaxExpressionParams; ++i) {
    e.parameters.set(kExpressionParamKeys[i], 0xFFFFFFFFu);
  }
  return e;
}

// A web document as the stored JSON of a lamp with `n` full expressions.
std::string document(size_t n, size_t nameLen = 8) {
  LampSettings lamp;
  lamp.name = std::string(nameLen, 'n');
  BaseSettings base;
  ShadeSettings shade;
  HomeModeSettings homeMode;
  ExpressionSettings expressions;
  for (size_t i = 0; i < n; ++i) expressions.expressions.push_back(full("pulse", 1 + i % 3));
  JsonDocument doc;
  config_codec::toJson(doc.to<JsonObject>(), lamp, base, shade, expressions, homeMode);
  std::string out;
  serializeJson(doc, out);
  return out;
}

}  // namespace

// The gate is set from these: a typical document (about 5 KB), every
// expression the caps allow, the 32 KB the web PUT accepts made of the
// smallest elements (the most slots per byte) and of one long string (the
// largest single block).
void test_raw_save_heap_figures_stay_inside_the_gate() {
  std::vector<std::string> docs = {document(10), document(kMaxExpressions)};
  std::string slots = "{\"lamp\":{},\"x\":[";
  while (slots.size() < 32760) slots += "0,";
  slots += "0]}";
  docs.push_back(slots);
  docs.push_back("{\"lamp\":{\"name\":\"" + std::string(32700, 'n') + "\"}}");
  bool inside = true;
  for (const auto& json : docs) {
    const Measured m = measure(json);
    const auto shape = config_codec::scanRawJson(json.c_str());
    printf("[raw] %zu B, %zu tokens, longest string %zu: peak %zu of %zu, largest %zu of %zu\n",
           m.len, shape.tokens, shape.longest, m.peak, shape.heapBytes(), m.largest,
           shape.largestBlock());
    inside &= m.peak <= shape.heapBytes() && m.largest <= shape.largestBlock();
  }
  TEST_ASSERT_TRUE(inside);
}

// A typical 5 KB document passes the gate at the runtime figures
// docs/dev/embedded-heap.md designs against: a 23.5 KB largest block in
// 46.5 KB free.
void test_a_typical_document_passes_at_the_documented_heap() {
  const std::string json = document(10);
  TEST_ASSERT_TRUE(json.size() >= 4500 && json.size() <= 6000);
  g_mock_largest_free_block = 23500;
  g_mock_free_size = 46500;
  InMemoryConfigStore s;
  TEST_ASSERT_TRUE(persist(s, json.c_str()) == SaveResult::Ok);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_truncated_blob_rejected_and_store_untouched);
  RUN_TEST(test_blob_without_lamp_object_rejected);
  RUN_TEST(test_valid_whole_document_saved_in_the_stored_shape);
  RUN_TEST(test_null_rejected);
  RUN_TEST(test_low_heap_refuses_before_parsing);
  RUN_TEST(test_raw_save_heap_figures_stay_inside_the_gate);
  RUN_TEST(test_a_typical_document_passes_at_the_documented_heap);
  return UNITY_END();
}
