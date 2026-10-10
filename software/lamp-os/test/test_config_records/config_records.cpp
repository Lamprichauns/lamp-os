// Native tests for the config storage: whole as one "cfg" string, or split
// into a compact record per expression, the knockout levels and the rest as
// blobs beside a lamp-only "cfg" string. Pins the
// round trip, the migration from the single-JSON form, a failed write, and the
// NVS budget at the caps (docs/dev/embedded-heap.md shows the arithmetic).

#include <unity.h>

#include <ArduinoJson.h>

#include <algorithm>
#include <string>
#include <vector>

#include "config/config_codec.hpp"
#include "config/disposition_store.hpp"
#include "paged_nvs.hpp"

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

// Every capped field at its ceiling; `fill` is the character the free-text
// fields are made of, so a '"' or '\\' fill gives the fully escaped worst case.
Model worstCaseModel(char fill = 'z') {
  Model m;
  m.lamp.name = std::string(kMaxLampNameLen, fill);
  m.lamp.password = std::string(kMaxPasswordLen, fill);
  // Every number at its widest and every boolean "false", the longer form.
  m.lamp.brightness = 255;
  m.lamp.apBootMinutes = 255;
  m.lamp.brightnessCeiling = 255;
  m.lamp.socialMode = SocialMode::Extrovert;
  m.lamp.setup = m.lamp.named = m.lamp.advancedEnabled = m.lamp.webappEnabled = false;
  m.base.colorsEditable = m.shade.colorsEditable = false;
  m.homeMode.brightness = 255;
  m.homeMode.enabled = m.homeMode.networkBound = m.homeMode.socialDisabled = false;
  m.lamp.lampType = "standard";
  for (auto* segs : {&m.base.segments, &m.shade.segments}) {
    segs->clear();
    for (size_t s = 0; s < kMaxSegments; ++s) {
      SegmentSettings seg;
      seg.name = std::string(kMaxSegmentNameLen, fill);
      seg.px = 255;
      seg.colors.assign(kMaxSurfaceColors, Color(255, 255, 255, 255));
      segs->push_back(seg);
    }
  }
  m.base.byteOrder = "GRBW";
  m.shade.byteOrder = "GRBW";
  m.base.knockoutPixels.assign(255, 100);
  m.homeMode.ssid = std::string(kMaxSsidLen, fill);
  m.homeMode.disabledExpressionTypes.assign(kMaxDisabledExpressionTypes,
                                            std::string(kMaxExpressionTypeLen, 't'));
  for (size_t i = 0; i < kMaxExpressions; ++i) {
    m.expressions.expressions.push_back(fullExpression(
        std::string(kMaxExpressionTypeLen, 'a' + i % 26).c_str(), 1 + i % 3));
  }
  return m;
}

SaveResult save(ConfigStore& store, const Model& m) {
  static char cfg[config_codec::kMaxStoredCfgJsonLen + 1];
  return config_codec::saveConfig(store, m.lamp, m.base, m.shade, m.expressions,
                                  m.homeMode, cfg, sizeof(cfg));
}

// The same load Config's constructor runs.
void load(ConfigStore& store, Model& m) {
  JsonDocument doc;
  deserializeJson(doc, store.read("cfg", "{}"));
  bool withinCaps = true;
  config_codec::loadStoredConfig(store, doc.as<JsonObject>(), m.lamp, m.base, m.shade,
                                 m.expressions, m.homeMode, withinCaps);
}

// Adds full expressions until the whole config cannot fit one cfg JSON, so the
// save splits into records.
void padToSplit(Model& m) {
  for (int i = 0; i < 4; ++i) {
    m.expressions.expressions.push_back(
        fullExpression(("pad" + std::to_string(i)).c_str(), 1));
  }
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

// A config can hold every key the table has; the record keeps only
// kMaxExpressionParams of them, so it never outgrows its bound.
void test_record_caps_parameters() {
  ExpressionConfig e = fullExpression("pulse", 1);
  for (size_t i = 0; i < kExpressionParamKeyCount; ++i) e.parameters.setIndex(i, 7);
  TEST_ASSERT_TRUE(e.parameters.size() > kMaxExpressionParams);
  uint8_t buf[config_codec::kMaxExpressionRecordLen + 64];
  const size_t len = config_codec::encodeExpression(e, buf);
  TEST_ASSERT_TRUE(len <= config_codec::kMaxExpressionRecordLen);
  ExpressionConfig out;
  TEST_ASSERT_TRUE(config_codec::decodeExpression(buf, len, out));
  TEST_ASSERT_EQUAL(kMaxExpressionParams, out.parameters.size());
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
// knockout, and a config that fits stays in that form.
void test_legacy_single_json_loads_and_small_config_stays_whole() {
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
  TEST_ASSERT_FALSE(store.has("xn") || store.has("x0") || store.has("ko"));
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(1, back.expressions.expressions.size());
  TEST_ASSERT_EQUAL_UINT32(4, back.expressions.expressions[0].getParameter("size", 0));
  TEST_ASSERT_EQUAL_UINT8(30, back.base.knockoutPixels[2]);
}

// A config too big for one cfg JSON splits into records; once it shrinks
// back, the whole save erases the split keys.
void test_split_then_whole_erases_split_keys() {
  InMemoryConfigStore store;
  Model m;
  m.base.segments = {{"Base", 3, {Color(1, 2, 3, 4)}}};
  m.base.knockoutPixels = {10, 20, 30};
  padToSplit(m);
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
  TEST_ASSERT_TRUE(store.has("xn") && store.has("x3") && store.has("ko"));
  TEST_ASSERT_TRUE(store.read("cfg", "").find("expressions") == std::string::npos);
  m.expressions.expressions.resize(1);
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
  TEST_ASSERT_FALSE(store.has("xn") || store.has("x0") || store.has("ko"));
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(1, back.expressions.expressions.size());
  TEST_ASSERT_EQUAL_UINT8(20, back.base.knockoutPixels[1]);
}

// A lamp flashed back to a build that predates records rewrites the cfg JSON
// with its own expressions and knockout; flashed forward again, that config
// loads and the stale records and knockout blob are ignored.
void test_config_saved_by_older_firmware_wins_over_stale_records() {
  InMemoryConfigStore store;
  Model m;
  m.base.segments = {{"Base", 3, {Color(1, 2, 3, 4)}}};
  m.base.knockoutPixels = {10, 20, 30};
  padToSplit(m);
  save(store, m);
  store.write("cfg",
              "{\"lamp\":{\"name\":\"jacko\"},\"base\":{\"segments\":[{\"name\":\"Base\","
              "\"px\":3,\"colors\":[\"#11223300\"]}],\"knockout\":[1,2,3]},"
              "\"expressions\":[{\"type\":\"spotty\",\"enabled\":true,\"target\":2}]}");
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL(1, back.expressions.expressions.size());
  TEST_ASSERT_EQUAL_STRING("spotty", back.expressions.expressions[0].type.c_str());
  TEST_ASSERT_EQUAL_UINT8(2, back.base.knockoutPixels[1]);
  TEST_ASSERT_TRUE(save(store, back) == SaveResult::Ok);
  TEST_ASSERT_FALSE(store.has("xn") || store.has("x0") || store.has("ko"));
}

// A cfg JSON that cannot fit is refused before any record, count or
// knockout write.
void test_too_large_writes_nothing() {
  InMemoryConfigStore store;
  Model m;
  padToSplit(m);
  char small[64];
  TEST_ASSERT_TRUE(config_codec::saveConfig(store, m.lamp, m.base, m.shade, m.expressions,
                                            m.homeMode, small, sizeof(small)) ==
                   SaveResult::TooLarge);
  TEST_ASSERT_EQUAL(0, store.writeCount);
}

// Over-long fields arriving through the cfg JSON (a web save or an old
// store) are bounded at parse, so the save that follows always fits.
void test_over_long_fields_cannot_fail_every_save() {
  const std::string big(5000, 'z');
  const std::string json =
      "{\"lamp\":{\"name\":\"" + big + "\",\"password\":\"" + big +
      "\"},\"base\":{\"byteOrder\":\"" + big + "\",\"segments\":[{\"name\":\"" + big +
      "\",\"px\":3,\"colors\":[\"#11223300\"]}]},\"shade\":{\"byteOrder\":\"" + big +
      "\"},\"homeMode\":{\"ssid\":\"" + big + "\",\"disabledExpressionTypes\":[\"" + big +
      "\"]}}";
  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
  Model m;
  config_codec::fromJson(doc.as<JsonObject>(), m.lamp, m.base, m.shade, m.expressions,
                         m.homeMode);
  TEST_ASSERT_EQUAL(kMaxPasswordLen, m.lamp.password.size());
  TEST_ASSERT_EQUAL(kMaxSsidLen, m.homeMode.ssid.size());
  TEST_ASSERT_TRUE(m.base.byteOrder.empty());
  TEST_ASSERT_TRUE(m.shade.byteOrder.empty());
  InMemoryConfigStore store;
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
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
  for (bool split : {false, true}) {
    Model before;
    before.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2),
                                      tagged("spotty", 3, 3), tagged("glitchy", 1, 4)};
    if (split) padToSplit(before);
    Model after = before;
    after.expressions.expressions.erase(after.expressions.expressions.begin());
    assertCrashSafe(before, after);
  }
}

void test_crash_mid_save_adding_and_editing_keeps_the_rest() {
  for (bool split : {false, true}) {
    Model before;
    before.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2)};
    if (split) padToSplit(before);
    Model after = before;
    after.expressions.expressions[1] = tagged("shifty", 2, 9);
    after.expressions.expressions.push_back(tagged("spotty", 3, 3));
    assertCrashSafe(before, after);
  }
}

// Growing past one cfg JSON, or shrinking back under it, is reset-safe too.
void test_crash_mid_save_crossing_the_split_keeps_the_rest() {
  Model small;
  small.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2)};
  Model big = small;
  padToSplit(big);
  assertCrashSafe(small, big);
  assertCrashSafe(big, small);
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

bool anyRecordLeft(InMemoryConfigStore& store) {
  for (int i = 0; i < 32; ++i) {
    if (store.has(("x" + std::to_string(i)).c_str())) return true;
  }
  return false;
}

// A whole save reset at any write, then a later whole save, leaves no record
// behind: the records go before the count, and every slot is swept.
void test_no_record_is_orphaned_by_a_reset() {
  Model big;
  padToSplit(big);
  big.expressions.expressions.push_back(tagged("pulse", 1, 1));
  Model small;
  small.expressions.expressions = {tagged("shifty", 2, 2)};
  for (size_t crashAt = 0; crashAt < 40; ++crashAt) {
    InMemoryConfigStore store;
    save(store, big);
    store.writesBeforeCrash = crashAt;
    save(store, small);
    store.writesBeforeCrash = SIZE_MAX;
    TEST_ASSERT_TRUE(save(store, small) == SaveResult::Ok);
    TEST_ASSERT_FALSE(anyRecordLeft(store));
    TEST_ASSERT_FALSE(store.has("xn"));
  }
}

// A split save cut short before its count leaves records past the old
// count; the next split save sweeps them.
void test_split_save_sweeps_records_past_its_count() {
  InMemoryConfigStore store;
  Model m;
  padToSplit(m);
  padToSplit(m);  // 8 full expressions: every save below stays split
  save(store, m);
  Model grown = m;
  for (int i = 0; i < 4; ++i) grown.expressions.expressions.push_back(tagged("spotty", 1 + i % 3, i));
  store.writesBeforeCrash = 11;  // records x0..x10 land, x11 and the count do not
  save(store, grown);
  store.writesBeforeCrash = SIZE_MAX;
  TEST_ASSERT_TRUE(store.has("x10"));
  Model shrunk = m;
  shrunk.expressions.expressions.pop_back();
  TEST_ASSERT_TRUE(save(store, shrunk) == SaveResult::Ok);
  TEST_ASSERT_TRUE(store.has("xn"));
  TEST_ASSERT_TRUE(store.has("x6"));
  for (int i = 7; i <= 10; ++i) {
    TEST_ASSERT_FALSE_MESSAGE(store.has(("x" + std::to_string(i)).c_str()),
                              std::to_string(i).c_str());
  }
}

// A record slot past every count this build writes (an earlier build's cap
// was higher) is swept by a whole save and by a split save alike.
void test_saves_sweep_record_slots_past_the_cap() {
  const uint8_t junk[3] = {1, 2, 3};
  for (bool split : {false, true}) {
    InMemoryConfigStore store;
    store.writeBlob("x25", junk, sizeof(junk));
    Model m;
    if (split) padToSplit(m);
    TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
    TEST_ASSERT_EQUAL(split, store.has("xn"));
    TEST_ASSERT_FALSE(store.has("x25"));
  }
}

// Deleting every expression saves whole with an empty expressions array; a
// reset before the old records are erased must still load none, never the
// records the delete removed.
void test_delete_all_survives_a_reset_before_the_record_erase() {
  Model before;
  padToSplit(before);
  Model none;
  for (size_t crashAt = 1; crashAt < 40; ++crashAt) {
    InMemoryConfigStore store;
    save(store, before);
    store.writesBeforeCrash = crashAt;
    const bool finished = save(store, none) == SaveResult::Ok;
    store.writesBeforeCrash = SIZE_MAX;
    TEST_ASSERT_TRUE(store.read("cfg", "").find("\"expressions\":[]") != std::string::npos);
    Model back;
    load(store, back);
    TEST_ASSERT_EQUAL(0, back.expressions.expressions.size());
    if (finished) return;
  }
}

// A whole cfg string is the authority, so a "cfgb" beside it (a save cut
// short, or left by a downgrade) goes at load: a split save would otherwise
// take the store for split already and skip staging the lamp section.
void test_load_of_a_whole_store_drops_a_stale_rest_blob() {
  InMemoryConfigStore store;
  Model m;
  m.lamp.name = "whole";
  save(store, m);
  store.writeBlob("cfgb", "{}", 2);
  Model back;
  load(store, back);
  TEST_ASSERT_EQUAL_STRING("whole", back.lamp.name.c_str());
  TEST_ASSERT_FALSE(store.has("cfgb"));
}

// A split store's cfg string is the lamp section alone, which an older
// firmware reads; the rest is the "cfgb" blob.
void test_split_cfg_string_is_the_lamp_section() {
  InMemoryConfigStore store;
  Model m;
  m.lamp.name = "split";
  padToSplit(m);
  padToSplit(m);
  TEST_ASSERT_TRUE(save(store, m) == SaveResult::Ok);
  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, store.read("cfg", "")) == DeserializationError::Ok);
  TEST_ASSERT_EQUAL_STRING("split", doc["lamp"]["name"] | "");
  TEST_ASSERT_EQUAL(1, doc.as<JsonObject>().size());
  TEST_ASSERT_TRUE(store.has("cfgb"));
  TEST_ASSERT_FALSE(store.has("cfgl"));
}

// An unreadable record drops only itself.
void test_unreadable_record_drops_only_itself() {
  InMemoryConfigStore store;
  Model m;
  m.expressions.expressions = {tagged("pulse", 1, 1), tagged("shifty", 2, 2),
                               tagged("spotty", 3, 3)};
  padToSplit(m);
  save(store, m);
  const uint8_t junk[3] = {9, 9, 9};
  store.writeBlob("x1", junk, sizeof(junk));
  Model back;
  JsonDocument doc;
  deserializeJson(doc, store.read("cfg", "{}"));
  config_codec::fromJson(doc.as<JsonObject>(), back.lamp, back.base, back.shade,
                         back.expressions, back.homeMode);
  const auto loaded = config_codec::loadRecords(store, back.base, back.expressions,
                                                false, false);
  TEST_ASSERT_TRUE(loaded.present);
  TEST_ASSERT_EQUAL_UINT32(1u << 1, loaded.droppedMask);
  TEST_ASSERT_EQUAL(6, back.expressions.expressions.size());
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

// NVS budget against the 20 KB partition: 5 pages of 126 entries, one held
// free for garbage collection. docs/dev/embedded-heap.md shows the arithmetic.
constexpr size_t kPageEntries = PagedNvs::kEntries;
constexpr size_t kDataPages = PagedNvs::kPages - 1;
constexpr size_t kMargin = 32;

// The keys outside the lamp namespace, as measured or bounded from each
// owner's store format. `pad` adds that many one-entry keys.
// - 5 namespace entries: lamp, crash, phy, nvs.net80211, nimble_bond;
// - PHY calibration: a 1904-byte cal_data blob, a 6-byte cal_mac blob and a
//   u32 cal_version;
// - the WiFi driver's namespace: empty, erased at boot (its config is in RAM);
// - the crash record: one blob under 64 bytes;
// - NimBLE's bond store (ble_store_nvs.c, bonding on, privacy on): our_sec_N
//   and peer_sec_N blobs of ble_store_value_sec (88 bytes on the ESP32) for
//   each of MAX_BONDS (3), p_dev_rec_N blobs of ble_hs_dev_records (44 bytes)
//   for MAX_BONDS + 1, cccd_sec_N blobs (16 bytes) for MAX_CCCDS (8),
//   csfc_sec_N (8 bytes) for MAX_CSFCS (1, the connection count), and
//   local_irk_N and rpa_rec_N (under 32 bytes) for MAX_BONDS each.
struct SystemKey {
  std::string key;
  size_t len;
};

std::vector<SystemKey> systemKeys() {
  std::vector<SystemKey> keys;
  for (const char* ns : {"ns/lamp", "ns/crash", "ns/phy", "ns/net80211", "ns/bond"}) {
    keys.push_back({ns, 0});
  }
  keys.push_back({"phy/cal_data", 1904});
  keys.push_back({"phy/cal_mac", 6});
  keys.push_back({"phy/cal_version", 0});
  keys.push_back({"crash/record", 64});
  for (int i = 0; i < 3; ++i) {
    keys.push_back({"bond/our_sec_" + std::to_string(i), 88});
    keys.push_back({"bond/peer_sec_" + std::to_string(i), 88});
    keys.push_back({"bond/local_irk_" + std::to_string(i), 32});
    keys.push_back({"bond/rpa_rec_" + std::to_string(i), 32});
  }
  for (int i = 0; i < 4; ++i) keys.push_back({"bond/p_dev_rec_" + std::to_string(i), 44});
  for (int i = 0; i < 8; ++i) keys.push_back({"bond/cccd_sec_" + std::to_string(i), 16});
  keys.push_back({"bond/csfc_sec_0", 8});
  return keys;
}

size_t systemEntries() {
  size_t n = 0;
  for (const auto& k : systemKeys()) n += k.len == 0 ? 1 : blobEntries(k.len);
  return n;
}

// The lamp namespace's keys besides the config: lampType and dispositions.
size_t lampExtraEntries() {
  return stringEntries(16) + blobEntries(DispositionStore::kMaxBlobLen);
}

enum class Part { Lamp, Rest, Whole };

std::string savedString(const Model& m, Part part) {
  static char buf[config_codec::kMaxStoredCfgJsonLen + 1];
  JsonWriter w(buf, sizeof(buf));
  if (part == Part::Lamp) {
    config_codec::writeLampOnlyJson(w, m.lamp);
  } else {
    config_codec::writeStoredJson(w, part == Part::Whole ? &m.lamp : nullptr, m.base, m.shade,
                                  m.homeMode, part == Part::Whole ? &m.expressions : nullptr);
  }
  w.finish();
  TEST_ASSERT_TRUE(w.ok());
  return std::string(buf);
}

// A string of `span` entries is placed when some page, compacted, has that
// many free: the page with the fewest live entries holds at most a quarter of
// the `live` entries on the four data pages. Returns the live entries that
// could still be added before that guarantee breaks.
long stringPlacementMargin(size_t live, size_t span) {
  return static_cast<long>(kDataPages * (kPageEntries - span) + kDataPages - 1) -
         static_cast<long>(live);
}

// The split keys but the cfg string: records, count, knockout, rest.
size_t splitKeyEntries(const Model& m) {
  uint8_t buf[config_codec::kMaxExpressionRecordLen];
  size_t records = 0;
  for (const auto& e : m.expressions.expressions) {
    records += blobEntries(config_codec::encodeExpression(e, buf));
  }
  return records + blobEntries(1) + blobEntries(m.base.knockoutPixels.size()) +
         blobEntries(savedString(m, Part::Rest).size());
}

// The whole cfg string at kMaxWholeCfgLen with everything else at its cap:
// rewritten over its old copy, or placed after a staged "cfgb" copy of the
// same size once the old string and the split keys are erased.
void test_whole_cfg_string_is_always_placed() {
  const size_t span = stringEntries(config_codec::kMaxWholeCfgLen);
  const size_t live = systemEntries() + lampExtraEntries() +
                      std::max(span, blobEntries(config_codec::kMaxWholeCfgLen));
  const long margin = stringPlacementMargin(live, span);
  printf("[budget] whole cfg: %zu entries, %zu live, margin %ld\n", span, live, margin);
  TEST_ASSERT_TRUE(margin >= static_cast<long>(kMargin));
}

// The split save's lamp string, rewritten over its old copy beside every
// split key at its cap, or placed after a staged "cfgl" copy once a whole
// string is erased.
void test_lamp_cfg_string_is_always_placed() {
  for (char fill : {'z', '"', '\\'}) {
    const Model m = worstCaseModel(fill);
    const size_t lampLen = savedString(m, Part::Lamp).size();
    const size_t span = stringEntries(lampLen);
    const size_t live = systemEntries() + lampExtraEntries() + splitKeyEntries(m) +
                        std::max(span, blobEntries(lampLen));
    const long margin = stringPlacementMargin(live, span);
    printf("[budget] lamp cfg '%c': %zu entries, %zu live, margin %ld\n", fill, span, live,
           margin);
    TEST_ASSERT_TRUE(margin >= static_cast<long>(kMargin));
  }
}

// A blob under 400 bytes is placed like a string: IDF writes its one chunk
// whole and moves to a fresh page rather than split it while the tail holds
// less than 400 bytes. The largest is a disposition blob just under that
// (14 entries), rewritten beside every split key at its cap.
void test_small_blobs_are_always_placed() {
  const size_t span = 1 + (399 + 31) / 32;
  for (char fill : {'z', '"', '\\'}) {
    const Model m = worstCaseModel(fill);
    const size_t live = systemEntries() + lampExtraEntries() + splitKeyEntries(m) +
                        stringEntries(savedString(m, Part::Lamp).size());
    const long margin = stringPlacementMargin(live, span);
    printf("[budget] small blob '%c': %zu entries, %zu live, margin %ld\n", fill, span, live,
           margin);
    TEST_ASSERT_TRUE(margin >= static_cast<long>(kMargin));
  }
}

// Blob rewrites need the new copy's entries before the old copy goes. The
// split peak is the larger of the rest blob and PHY calibration (1904 bytes)
// rewritten beside every split key at its cap; the switch from whole adds the
// split keys beside the whole cfg string, and a lamp string NVS could not
// place there adds its staged copy.
void test_split_peaks_fit_the_entries() {
  for (char fill : {'z', '"', '\\'}) {
    const Model m = worstCaseModel(fill);
    const size_t lampLen = savedString(m, Part::Lamp).size();
    const size_t rewrite =
        std::max(blobEntries(savedString(m, Part::Rest).size()), blobEntries(1904));
    const size_t base = systemEntries() + lampExtraEntries() + splitKeyEntries(m);
    const size_t steady = base + stringEntries(lampLen) + rewrite;
    const size_t switching =
        base + stringEntries(config_codec::kMaxWholeCfgLen) + blobEntries(lampLen);
    printf("[budget] split '%c': steady peak %zu, switch peak %zu of %zu\n", fill, steady,
           switching, kDataPages * kPageEntries);
    TEST_ASSERT_TRUE(steady + kMargin <= kDataPages * kPageEntries);
    TEST_ASSERT_TRUE(switching + kMargin <= kDataPages * kPageEntries);
  }
}

// The largest whole config: full expressions until the next would pass
// kMaxWholeCfgLen.
Model largestWholeModel() {
  Model m;
  m.lamp.name = std::string(kMaxLampNameLen, 'w');
  static char buf[config_codec::kMaxWholeCfgLen + 1];
  for (size_t i = 0; i < kMaxExpressions; ++i) {
    m.expressions.expressions.push_back(fullExpression("pulse", 1 + i % 3));
    JsonWriter w(buf, sizeof(buf));
    config_codec::writeStoredJson(w, &m.lamp, m.base, m.shade, m.homeMode, &m.expressions);
    w.finish();
    if (!w.ok()) {
      m.expressions.expressions.pop_back();
      break;
    }
  }
  TEST_ASSERT_TRUE(m.expressions.expressions.size() > 1);
  return m;
}

uint32_t nextRandom(uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return state >> 8;
}

// The fields each storage key carries, compared after a load.
bool sameLamp(const Model& a, const Model& b) {
  return a.lamp.name == b.lamp.name && a.lamp.password == b.lamp.password &&
         a.lamp.brightness == b.lamp.brightness;
}
bool sameRest(const Model& a, const Model& b) {
  return a.base.segments.size() == b.base.segments.size() &&
         a.base.knockoutPixels == b.base.knockoutPixels && a.homeMode.ssid == b.homeMode.ssid;
}
bool sameExpressions(const Model& a, const Model& b) {
  const auto& x = a.expressions.expressions;
  const auto& y = b.expressions.expressions;
  if (x.size() != y.size()) return false;
  for (size_t i = 0; i < x.size(); ++i) {
    if (x[i].type != y[i].type || x[i].intervalMin != y[i].intervalMin) return false;
  }
  return true;
}

// Fails writes and erases past `budget`, the way a reset mid-save leaves NVS.
class CrashAfter : public ConfigStore {
 public:
  CrashAfter(ConfigStore& inner, size_t budget) : inner_(inner), budget_(budget) {}
  std::string read(const char* key, const char* d) override { return inner_.read(key, d); }
  size_t readBlob(const char* key, void* out, size_t cap) override {
    return inner_.readBlob(key, out, cap);
  }
  size_t write(const char* key, const char* value) override {
    return spend() ? inner_.write(key, value) : 0;
  }
  size_t writeBlob(const char* key, const void* data, size_t len) override {
    return spend() ? inner_.writeBlob(key, data, len) : 0;
  }
  bool erase(const char* key) override { return spend() && inner_.erase(key); }
  bool clear() override { return inner_.clear(); }
  size_t spent = 0;

 private:
  bool spend() {
    if (spent == budget_) return false;
    ++spent;
    return true;
  }
  ConfigStore& inner_;
  size_t budget_;
};

struct Churn {
  size_t wholeStaged = 0;  // whole saves whose string NVS refused at first
  size_t toSplit = 0;      // saves switching a whole store to split
  size_t crashChecks = 0;
};

bool isWhole(PagedNvs& nvs) {
  return nvs.read("cfg", "").find("\"expressions\"") != std::string::npos;
}

// A save that took a staged path, cut short after every write it makes: each
// load must give back the lamp section, the rest and the expressions each
// either as before or as saved.
bool survivesCrashes(const PagedNvs& before, const Model& old, const Model& m, Churn& churn) {
  for (size_t budget = 0;; ++budget) {
    PagedNvs nvs = before;
    CrashAfter crashing(nvs, budget);
    const bool finished = save(crashing, m) == SaveResult::Ok && crashing.spent < budget;
    Model back;
    load(nvs, back);
    ++churn.crashChecks;
    if (!(sameLamp(back, old) || sameLamp(back, m)) ||
        !(sameRest(back, old) || sameRest(back, m)) ||
        !(sameExpressions(back, old) || sameExpressions(back, m))) {
      return false;
    }
    if (finished) return true;
  }
}

// Saves through every shape the store takes (whole, the largest whole, split
// at every escaped worst case, the standard variant) in a seeded order, with
// the system keys and dispositions rewritten between saves the way bonding,
// boots and dispositions churn them, and `pad` one-entry keys beside them.
// Every write but a refused string must land, and every save must load back
// as saved; with `crashes`, a save that took a staged path is also cut short
// after each of its writes.
bool churnFits(size_t pad, uint32_t seed, size_t rounds, bool crashes, Churn& churn) {
  PagedNvs nvs;
  const auto system = systemKeys();
  for (const auto& k : system) {
    if (!nvs.putSystem(k.key, k.len)) return false;
  }
  for (size_t i = 0; i < pad; ++i) {
    if (!nvs.putSystem("pad/" + std::to_string(i), 0)) return false;
  }
  if (nvs.write("lampType", "standard") == 0) return false;

  Model standard = worstCaseModel();
  standard.expressions.expressions.clear();
  for (const char* type : {"glitchy", "pulse", "breathing", "shifty", "spotty", "flicker"}) {
    for (uint8_t target = 1; target <= 3; ++target) {
      standard.expressions.expressions.push_back(fullExpression(type, target));
    }
  }
  // Each shape as a load gives it back, so a comparison sees only storage.
  std::vector<Model> shapes;
  for (const Model& m : {Model{}, largestWholeModel(), worstCaseModel('z'), worstCaseModel('"'),
                         worstCaseModel('\\'), standard}) {
    InMemoryConfigStore canonical;
    save(canonical, m);
    shapes.emplace_back();
    load(canonical, shapes.back());
  }
  uint8_t disp[DispositionStore::kMaxBlobLen];
  Model old;
  load(nvs, old);
  for (size_t round = 0; round < rounds; ++round) {
    Model m = shapes[nextRandom(seed) % shapes.size()];
    m.lamp.brightness = static_cast<uint8_t>(round);
    if (!m.expressions.expressions.empty()) {
      m.expressions.expressions[round % m.expressions.expressions.size()].intervalMin =
          static_cast<uint32_t>(round);
    }
    const PagedNvs before = nvs;
    const size_t refused = nvs.failedWrites;
    const bool wasWhole = isWhole(nvs);
    if (save(nvs, m) != SaveResult::Ok) return false;
    const bool staged = nvs.failedWrites != refused;
    const bool switched = wasWhole && !isWhole(nvs);
    churn.wholeStaged += staged;
    churn.toSplit += switched;
    if (crashes && (staged || switched) && !survivesCrashes(before, old, m, churn)) {
      return false;
    }
    Model back;
    load(nvs, back);
    if (!sameLamp(back, m) || !sameRest(back, m) || !sameExpressions(back, m)) return false;
    old = back;
    const auto& k = system[nextRandom(seed) % system.size()];
    if (!nvs.putSystem(k.key, k.len, static_cast<uint32_t>(round))) return false;
    std::memset(disp, static_cast<int>(round), sizeof(disp));
    if (nvs.writeBlob("disp", disp, nextRandom(seed) % sizeof(disp) + 1) == 0) return false;
  }
  return true;
}

// The page model refuses a string no page has room for, though the entries
// free across the pages would hold it, and spreads a blob of the same size.
void test_page_model_confines_a_string_to_one_page() {
  PagedNvs nvs;
  for (size_t i = 0; i < kDataPages * kPageEntries; ++i) {
    TEST_ASSERT_TRUE(nvs.putSystem("p/" + std::to_string(i), 0));
  }
  for (size_t page = 0; page < kDataPages; ++page) {
    for (size_t i = 0; i < 20; ++i) {
      nvs.eraseSystem("p/" + std::to_string(page * kPageEntries + i));
    }
  }
  TEST_ASSERT_EQUAL(kDataPages * (kPageEntries - 20), nvs.liveEntries());
  const std::string thirty(29 * 32 - 1, 's');
  TEST_ASSERT_EQUAL(0, nvs.write("s", thirty.c_str()));
  TEST_ASSERT_EQUAL(thirty.size(), nvs.writeBlob("b", thirty.data(), thirty.size()));
  std::string back(thirty.size(), '\0');
  TEST_ASSERT_EQUAL(thirty.size(), nvs.readBlob("b", &back[0], back.size()));
  TEST_ASSERT_TRUE(back == thirty);
}

void test_every_store_shape_survives_churn_with_margin() {
  Churn churn;
  // Seed 31 leaves the least room of the first 150 (34 spare entries); seed
  // 69 switches to split where no page has room for the lamp string beside
  // the whole one.
  for (uint32_t seed : {1u, 7u, 31u, 42u, 69u}) {
    TEST_ASSERT_TRUE(churnFits(kMargin, seed, 400, true, churn));
  }
  printf("[budget] churn: %zu staged whole saves, %zu switches to split, %zu crash points\n",
         churn.wholeStaged, churn.toSplit, churn.crashChecks);
  TEST_ASSERT_TRUE(churn.wholeStaged > 0);
  TEST_ASSERT_TRUE(churn.toSplit > 0);
  size_t pad = kMargin;
  while (churnFits(pad + 1, 1, 400, false, churn)) ++pad;
  printf("[budget] churn holds with %zu spare entries\n", pad);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_record_round_trip);
  RUN_TEST(test_worst_record_fits_the_bound);
  RUN_TEST(test_truncated_record_rejected);
  RUN_TEST(test_record_caps_parameters);
  RUN_TEST(test_save_load_round_trip);
  RUN_TEST(test_legacy_single_json_loads_and_small_config_stays_whole);
  RUN_TEST(test_split_then_whole_erases_split_keys);
  RUN_TEST(test_config_saved_by_older_firmware_wins_over_stale_records);
  RUN_TEST(test_too_large_writes_nothing);
  RUN_TEST(test_no_record_is_orphaned_by_a_reset);
  RUN_TEST(test_delete_all_survives_a_reset_before_the_record_erase);
  RUN_TEST(test_split_save_sweeps_records_past_its_count);
  RUN_TEST(test_saves_sweep_record_slots_past_the_cap);
  RUN_TEST(test_over_long_fields_cannot_fail_every_save);
  RUN_TEST(test_removed_expressions_erase_stale_records);
  RUN_TEST(test_failed_write_reports_and_keeps_the_old_config);
  RUN_TEST(test_page_model_confines_a_string_to_one_page);
  RUN_TEST(test_whole_cfg_string_is_always_placed);
  RUN_TEST(test_lamp_cfg_string_is_always_placed);
  RUN_TEST(test_small_blobs_are_always_placed);
  RUN_TEST(test_split_peaks_fit_the_entries);
  RUN_TEST(test_every_store_shape_survives_churn_with_margin);
  RUN_TEST(test_crash_mid_save_removing_an_entry_keeps_the_rest);
  RUN_TEST(test_crash_mid_save_adding_and_editing_keeps_the_rest);
  RUN_TEST(test_crash_mid_save_crossing_the_split_keeps_the_rest);
  RUN_TEST(test_crash_mid_migration_keeps_the_legacy_config);
  RUN_TEST(test_load_of_a_whole_store_drops_a_stale_rest_blob);
  RUN_TEST(test_split_cfg_string_is_the_lamp_section);
  RUN_TEST(test_unreadable_record_drops_only_itself);
  RUN_TEST(test_cfg_json_over_the_buffer_fails_cleanly);
  RUN_TEST(test_unregistered_type_record_loads);
  return UNITY_END();
}
