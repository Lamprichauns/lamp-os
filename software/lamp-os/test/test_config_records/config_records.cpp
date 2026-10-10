// Native tests for the split config storage: each expression as a compact
// record, knockout levels as a blob, the rest as the "cfg" JSON. Pins the
// round trip, the migration from the single-JSON form, a failed write, and the
// NVS budget at the caps (docs/dev/embedded-heap.md shows the arithmetic).

#include <unity.h>

#include <ArduinoJson.h>

#include <string>

#include "config/config_codec.hpp"
#include "config/disposition_store.hpp"

#include "../../src/util/color.cpp"
#include "../../src/util/heap_guard.cpp"
#include "../../src/config/config_codec.cpp"

using namespace lamp;
using config_codec::SaveResult;

void setUp(void) {}
void tearDown(void) {}

namespace {

struct Model {
  LampSettings lamp;
  BaseSettings base;
  ShadeSettings shade;
  ExpressionSettings expressions;
  HomeModeSettings homeMode;
};

ExpressionConfig fullExpression(const char* type, uint8_t target) {
  ExpressionConfig e;
  e.type = type;
  e.enabled = true;
  e.target = target;
  e.intervalMin = 0xFFFFFFFFu;
  e.intervalMax = 0xFFFFFFFFu;
  for (size_t i = 0; i < kMaxExpressionColors; ++i) {
    e.colors.push_back(Color(static_cast<uint8_t>(i), 2, 3, 4));
  }
  for (size_t i = 0; i < kMaxExpressionParams; ++i) {
    e.parameters[kExpressionParamKeys[i]] = 0xFFFFFFFFu;
  }
  return e;
}

// Every capped field at its ceiling.
Model worstCaseModel() {
  Model m;
  m.lamp.name = std::string(kMaxLampNameLen, 'n');
  m.lamp.password = std::string(kMaxPasswordLen, 'p');
  m.lamp.lampType = "standard";
  for (auto* segs : {&m.base.segments, &m.shade.segments}) {
    segs->clear();
    for (size_t s = 0; s < kMaxSegments; ++s) {
      SegmentSettings seg;
      seg.name = std::string(kMaxSegmentNameLen, 's');
      seg.px = 63;
      seg.colors.assign(kMaxSurfaceColors, Color(255, 255, 255, 255));
      segs->push_back(seg);
    }
  }
  m.base.byteOrder = "GRBW";
  m.shade.byteOrder = "GRBW";
  m.base.knockoutPixels.assign(m.base.sumPx(), 100);
  m.homeMode.ssid = std::string(kMaxSsidLen, 'w');
  m.homeMode.disabledExpressionTypes.assign(kMaxDisabledExpressionTypes,
                                            std::string(kMaxExpressionTypeLen, 't'));
  for (size_t i = 0; i < kMaxExpressions; ++i) {
    m.expressions.expressions.push_back(fullExpression(
        std::string(kMaxExpressionTypeLen, 'a' + i % 26).c_str(), 1 + i % 3));
  }
  return m;
}

SaveResult save(InMemoryConfigStore& store, const Model& m) {
  return config_codec::saveConfig(store, m.lamp, m.base, m.shade, m.expressions,
                                  m.homeMode);
}

void load(InMemoryConfigStore& store, Model& m) {
  JsonDocument doc;
  deserializeJson(doc, store.read("cfg", "{}"));
  config_codec::fromJson(doc.as<JsonObject>(), m.lamp, m.base, m.shade,
                         m.expressions, m.homeMode);
  config_codec::loadRecords(store, m.base, m.expressions);
}

// NVS stores 32-byte entries, 126 to a 4 KB page. A string is a header entry
// plus its bytes (with the NUL); a blob adds an index entry to that.
size_t stringEntries(size_t len) { return 1 + (len + 1 + 31) / 32; }
size_t blobEntries(size_t len) { return 2 + (len + 31) / 32; }

}  // namespace

void test_record_round_trip() {
  ExpressionConfig in = fullExpression("shifty", 2);
  uint8_t buf[config_codec::kMaxExpressionRecordLen];
  const size_t len = config_codec::encodeExpression(in, buf);
  TEST_ASSERT_EQUAL(4 + 6 + 8 + 1 + 32 + 1 + 5 * kMaxExpressionParams, len);
  ExpressionConfig out;
  TEST_ASSERT_TRUE(config_codec::decodeExpression(buf, len, out));
  TEST_ASSERT_EQUAL_STRING("shifty", out.type.c_str());
  TEST_ASSERT_TRUE(out.enabled);
  TEST_ASSERT_EQUAL_UINT8(2, out.target);
  TEST_ASSERT_EQUAL_UINT32(in.intervalMin, out.intervalMin);
  TEST_ASSERT_TRUE(in.colors == out.colors);
  TEST_ASSERT_TRUE(in.parameters == out.parameters);
}

void test_worst_record_fits_the_bound() {
  ExpressionConfig in = fullExpression("aaaaaaaaaaaaaaa", 3);
  uint8_t buf[config_codec::kMaxExpressionRecordLen];
  TEST_ASSERT_EQUAL(config_codec::kMaxExpressionRecordLen,
                    config_codec::encodeExpression(in, buf));
}

void test_truncated_record_rejected() {
  ExpressionConfig in = fullExpression("pulse", 1);
  uint8_t buf[config_codec::kMaxExpressionRecordLen];
  const size_t len = config_codec::encodeExpression(in, buf);
  ExpressionConfig out;
  for (size_t cut = 0; cut < len; ++cut) {
    TEST_ASSERT_FALSE(config_codec::decodeExpression(buf, cut, out));
  }
}

void test_save_load_round_trip() {
  InMemoryConfigStore store;
  Model m = worstCaseModel();
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(m.expressions.expressions.size(), back.expressions.expressions.size());
  for (size_t i = 0; i < m.expressions.expressions.size(); ++i) {
    const auto& a = m.expressions.expressions[i];
    const auto& b = back.expressions.expressions[i];
    TEST_ASSERT_EQUAL_STRING(a.type.c_str(), b.type.c_str());
    TEST_ASSERT_TRUE(a.parameters == b.parameters);
    TEST_ASSERT_TRUE(a.colors == b.colors);
  }
  TEST_ASSERT_TRUE(m.base.knockoutPixels == back.base.knockoutPixels);
  TEST_ASSERT_EQUAL_STRING(m.lamp.password.c_str(), back.lamp.password.c_str());
  TEST_ASSERT_FALSE(store.read("cfg", "").find("expressions") != std::string::npos);
  TEST_ASSERT_FALSE(store.read("cfg", "").find("knockout") != std::string::npos);
}

// A lamp updating from the single-JSON form keeps its expressions and
// knockout; the first save moves them into records.
void test_legacy_single_json_migrates_on_save() {
  InMemoryConfigStore store;
  store.write("cfg",
              "{\"lamp\":{\"name\":\"jacko\"},\"base\":{\"segments\":[{\"name\":\"Base\","
              "\"px\":3,\"colors\":[\"#11223300\"]}],\"knockout\":[10,20,30]},"
              "\"expressions\":[{\"type\":\"pulse\",\"enabled\":true,\"target\":1,"
              "\"size\":4,\"colors\":[\"#ff000000\"]}]}");
  Model m;
  load(store, m);
  TEST_ASSERT_EQUAL(1, m.expressions.expressions.size());
  TEST_ASSERT_EQUAL_UINT8(20, m.base.knockoutPixels[1]);
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
  TEST_ASSERT_TRUE(store.has("xn") && store.has("x0") && store.has("ko"));
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(1, back.expressions.expressions.size());
  TEST_ASSERT_EQUAL_UINT32(4, back.expressions.expressions[0].getParameter("size", 0));
  TEST_ASSERT_EQUAL_UINT8(30, back.base.knockoutPixels[2]);
}

void test_removed_expressions_erase_stale_records() {
  InMemoryConfigStore store;
  Model m;
  m.expressions.expressions = {fullExpression("pulse", 1), fullExpression("shifty", 2)};
  save(store, m);
  m.expressions.expressions.pop_back();
  save(store, m);
  TEST_ASSERT_FALSE(store.has("x1"));
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(1, back.expressions.expressions.size());
}

// A full NVS returns a write failure, never a fault, and leaves the stored
// config readable.
void test_failed_write_reports_and_keeps_the_old_config() {
  InMemoryConfigStore store;
  Model m;
  m.lamp.name = "jacko";
  m.expressions.expressions = {fullExpression("pulse", 1)};
  save(store, m);
  store.failWrites = true;
  m.lamp.name = "other";
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::WriteFailed);
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL_STRING("jacko", back.lamp.name.c_str());
}

// Worst-case NVS use at the caps, against the 20 KB partition: 5 pages, one
// held free for garbage collection, 126 entries each. The system keys are
// measured ceilings (PHY calibration is a 1904-byte blob; the WiFi driver's
// namespace stays under 64 entries), and a rewrite needs room for a second
// copy of the largest item before the old one is erased.
void test_worst_case_config_fits_nvs_with_margin() {
  InMemoryConfigStore store;
  const Model m = worstCaseModel();
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);

  const size_t cfgLen = store.read("cfg", "").size();
  const size_t record = blobEntries(config_codec::kMaxExpressionRecordLen);
  const size_t lampNs = kMaxExpressions * record + blobEntries(1) +
                        blobEntries(255) + stringEntries(cfgLen) +
                        stringEntries(16) +  // lampType
                        blobEntries(DispositionStore::kMaxBlobLen);
  const size_t system = 4 /* namespaces */ + blobEntries(1904) + blobEntries(6) +
                        1 /* phy cal_version */ + 64 /* nvs.net80211 */ +
                        blobEntries(64) /* crash record */;
  const size_t largest = std::max(stringEntries(cfgLen),
                                  blobEntries(DispositionStore::kMaxBlobLen));
  const size_t usable = (5 - 1) * 126;
  const size_t needed = lampNs + system + largest;
  printf("[budget] cfg=%zuB lamp=%zu system=%zu rewrite=%zu needed=%zu of %zu\n",
         cfgLen, lampNs, system, largest, needed, usable);
  TEST_ASSERT_TRUE(cfgLen < 4000);  // one NVS string must fit one page
  TEST_ASSERT_TRUE(needed + 32 <= usable);  // at least 1 KB spare
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_record_round_trip);
  RUN_TEST(test_worst_record_fits_the_bound);
  RUN_TEST(test_truncated_record_rejected);
  RUN_TEST(test_save_load_round_trip);
  RUN_TEST(test_legacy_single_json_migrates_on_save);
  RUN_TEST(test_removed_expressions_erase_stale_records);
  RUN_TEST(test_failed_write_reports_and_keeps_the_old_config);
  RUN_TEST(test_worst_case_config_fits_nvs_with_margin);
  return UNITY_END();
}
