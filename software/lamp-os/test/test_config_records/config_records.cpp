// Native tests for the split config storage: each expression as a compact
// record, knockout levels as a blob, the rest as the "cfg" JSON. Pins the
// round trip, the migration from the single-JSON form, a failed write, and the
// NVS budget at the caps (docs/dev/embedded-heap.md shows the arithmetic).

#include <unity.h>

#include <ArduinoJson.h>

#include <algorithm>
#include <string>
#include <vector>

#include "config/config_codec.hpp"
#include "config/disposition_store.hpp"

#include "../../src/util/color.cpp"
#include "../../src/util/heap_guard.cpp"
#include "../../src/util/json_writer.cpp"
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
    e.parameters.set(kExpressionParamKeys[i], 0xFFFFFFFFu);
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
  static char cfg[config_codec::kMaxStoredCfgJsonLen + 1];
  return config_codec::saveConfig(store, m.lamp, m.base, m.shade, m.expressions,
                                  m.homeMode, cfg, sizeof(cfg));
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

namespace {

std::vector<std::pair<std::string, uint8_t>> keysOf(const ExpressionSettings& s) {
  std::vector<std::pair<std::string, uint8_t>> keys;
  for (const auto& e : s.expressions) keys.push_back({e.type, e.target});
  std::sort(keys.begin(), keys.end());
  return keys;
}

ExpressionConfig tagged(const char* type, uint8_t target, uint32_t size) {
  ExpressionConfig e;
  e.type = type;
  e.target = target;
  e.colors = {Color(1, 2, 3, 4)};
  e.setParameter("size", size);
  return e;
}

// Saves `before`, then saves `after` with the store reset partway through,
// at every possible write; each load must be a whole config: every entry is
// one `before` or `after` held (same type, target and contents), every entry
// both hold is present, and nothing from neither appears.
void assertCrashSafe(const Model& before, const Model& after) {
  for (size_t crashAt = 0; crashAt < 64; ++crashAt) {
    InMemoryConfigStore store;
    save(store, before);
    store.writesBeforeCrash = crashAt;
    const bool finished = save(store, after) == SaveResult::Ok;
    store.writesBeforeCrash = SIZE_MAX;
    Model back;
    load(store, back);
    auto keys = keysOf(back.expressions);
    TEST_ASSERT_TRUE_MESSAGE(std::adjacent_find(keys.begin(), keys.end()) == keys.end(),
                             "an expression loaded twice");
    for (const auto& e : back.expressions.expressions) {
      auto matches = [&](const Model& m) {
        for (const auto& o : m.expressions.expressions) {
          if (o.type == e.type && o.target == e.target && o.parameters == e.parameters) return true;
        }
        return false;
      };
      TEST_ASSERT_TRUE_MESSAGE(matches(before) || matches(after), e.type.c_str());
    }
    for (const auto& o : before.expressions.expressions) {
      bool inAfter = false;
      for (const auto& a : after.expressions.expressions) {
        inAfter |= a.type == o.type && a.target == o.target;
      }
      if (!inAfter) continue;
      bool loaded = false;
      for (const auto& e : back.expressions.expressions) {
        loaded |= e.type == o.type && e.target == o.target;
      }
      TEST_ASSERT_TRUE_MESSAGE(loaded, o.type.c_str());
    }
    if (finished) {
      TEST_ASSERT_TRUE(keysOf(back.expressions) == keysOf(after.expressions));
      return;
    }
  }
  TEST_FAIL_MESSAGE("save never finished");
}

}  // namespace

void test_crash_mid_save_removing_an_entry_keeps_the_rest() {
  Model before;
  before.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2),
                                    tagged("spotty", 3, 3), tagged("glitchy", 1, 4)};
  Model after = before;
  after.expressions.expressions.erase(after.expressions.expressions.begin());
  assertCrashSafe(before, after);
}

void test_crash_mid_save_adding_and_editing_keeps_the_rest() {
  Model before;
  before.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2)};
  Model after = before;
  after.expressions.expressions[1] = tagged("shifty", 2, 9);
  after.expressions.expressions.push_back(tagged("spotty", 3, 3));
  assertCrashSafe(before, after);
}

// A reset during the first save after an update leaves the single-JSON form
// readable: the records only take over once their count is written, and the
// legacy JSON is rewritten last.
void test_crash_mid_migration_keeps_the_legacy_config() {
  for (size_t crashAt = 0; crashAt < 64; ++crashAt) {
    InMemoryConfigStore store;
    store.write("cfg",
                "{\"lamp\":{\"name\":\"jacko\"},\"expressions\":["
                "{\"type\":\"pulse\",\"enabled\":true,\"target\":1,\"size\":4},"
                "{\"type\":\"shifty\",\"enabled\":true,\"target\":2}]}");
    Model m;
    load(store, m);
    store.writesBeforeCrash = crashAt;
    const bool finished = save(store, m) == SaveResult::Ok;
    store.writesBeforeCrash = SIZE_MAX;
    Model back;
    load(store, back);
    TEST_ASSERT_EQUAL_STRING("jacko", back.lamp.name.c_str());
    TEST_ASSERT_EQUAL(2, back.expressions.expressions.size());
    if (finished) return;
  }
  TEST_FAIL_MESSAGE("save never finished");
}

// An unreadable record drops itself and nothing else.
// A cfg JSON that outgrows the fixed save buffer fails the save and leaves
// the stored cfg as it was.
void test_cfg_json_over_the_buffer_fails_cleanly() {
  InMemoryConfigStore store;
  Model m;
  m.lamp.name = "jacko";
  save(store, m);
  const std::string before = store.read("cfg", "");
  char small[64];
  m.lamp.name = "other";
  TEST_ASSERT_TRUE(config_codec::saveConfig(store, m.lamp, m.base, m.shade, m.expressions,
                                            m.homeMode, small, sizeof(small)) ==
                   SaveResult::TooLarge);
  TEST_ASSERT_EQUAL_STRING(before.c_str(), store.read("cfg", "").c_str());
}

void test_unreadable_record_drops_only_itself() {
  InMemoryConfigStore store;
  Model m;
  m.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2),
                               tagged("spotty", 3, 3)};
  save(store, m);
  const uint8_t junk[3] = {9, 9, 9};
  store.writeBlob("x1", junk, sizeof(junk));
  Model back;
  JsonDocument doc;
  deserializeJson(doc, store.read("cfg", "{}"));
  config_codec::fromJson(doc.as<JsonObject>(), back.lamp, back.base, back.shade,
                         back.expressions, back.homeMode);
  const auto loaded = config_codec::loadRecords(store, back.base, back.expressions);
  TEST_ASSERT_TRUE(loaded.present);
  TEST_ASSERT_EQUAL_UINT32(1u << 1, loaded.droppedMask);
  TEST_ASSERT_EQUAL(2, back.expressions.expressions.size());
  TEST_ASSERT_EQUAL_STRING("pulse", back.expressions.expressions[0].type.c_str());
  TEST_ASSERT_EQUAL_STRING("spotty", back.expressions.expressions[1].type.c_str());
}

// A type the running variant does not register still loads: it may belong to
// the variant the lamp is flashed back to, and dropping it would lose it.
void test_unregistered_type_record_loads() {
  InMemoryConfigStore store;
  Model m;
  m.expressions.expressions = {tagged("bloom", 3, 1), tagged("pulse", 1, 2)};
  save(store, m);
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(2, back.expressions.expressions.size());
}

// Worst-case NVS use against the 20 KB partition: 5 pages, one held free for
// garbage collection, 126 entries each. The system keys are measured ceilings
// (PHY calibration is a 1904-byte blob; the WiFi driver's namespace stays under
// 64 entries), and a rewrite needs room for a second copy of the largest item
// before the old one is erased.
constexpr unsigned kUsableEntries = (5 - 1) * 126;

size_t nvsEntriesNeeded(const Model& m, const char* label) {
  InMemoryConfigStore store;
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
  const size_t cfgLen = store.read("cfg", "").size();
  size_t records = 0;
  uint8_t buf[config_codec::kMaxExpressionRecordLen];
  for (const auto& e : m.expressions.expressions) {
    records += blobEntries(config_codec::encodeExpression(e, buf));
  }
  const size_t lampNs = records + blobEntries(1) + blobEntries(255) +
                        stringEntries(cfgLen) + stringEntries(16) /* lampType */ +
                        blobEntries(DispositionStore::kMaxBlobLen);
  const size_t system = 4 /* namespaces */ + blobEntries(1904) + blobEntries(6) +
                        1 /* phy cal_version */ + 64 /* nvs.net80211 */ +
                        blobEntries(64) /* crash record */;
  const size_t largest = std::max(stringEntries(cfgLen),
                                  blobEntries(DispositionStore::kMaxBlobLen));
  const size_t needed = lampNs + system + largest;
  printf("[budget] %s: %zu expressions in %zu entries, cfg=%zuB, lamp=%zu "
         "system=%zu rewrite=%zu needed=%zu of %u\n",
         label, m.expressions.expressions.size(), records, cfgLen, lampNs, system,
         largest, needed, kUsableEntries);
  TEST_ASSERT_TRUE(cfgLen <= config_codec::kMaxStoredCfgJsonLen);
  return needed;
}

void test_worst_case_config_fits_nvs_with_margin() {
  TEST_ASSERT_TRUE(nvsEntriesNeeded(worstCaseModel(), "caps") + 32 <= kUsableEntries);
}

// The standard variant's real worst case: its six registered types at every
// target, every expression full.
void test_standard_variant_worst_case_fits_nvs() {
  Model m = worstCaseModel();
  m.expressions.expressions.clear();
  for (const char* type : {"glitchy", "pulse", "breathing", "shifty", "spotty", "flicker"}) {
    for (uint8_t target = 1; target <= 3; ++target) {
      m.expressions.expressions.push_back(fullExpression(type, target));
    }
  }
  TEST_ASSERT_TRUE(nvsEntriesNeeded(m, "standard") + 32 <= kUsableEntries);
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
  RUN_TEST(test_standard_variant_worst_case_fits_nvs);
  RUN_TEST(test_crash_mid_save_removing_an_entry_keeps_the_rest);
  RUN_TEST(test_crash_mid_save_adding_and_editing_keeps_the_rest);
  RUN_TEST(test_crash_mid_migration_keeps_the_legacy_config);
  RUN_TEST(test_unreadable_record_drops_only_itself);
  RUN_TEST(test_cfg_json_over_the_buffer_fails_cleanly);
  RUN_TEST(test_unregistered_type_record_loads);
  return UNITY_END();
}
