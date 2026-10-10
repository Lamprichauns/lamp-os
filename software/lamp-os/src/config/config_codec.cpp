#include "config/config_codec.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <lampos/led_types.hpp>

#include "util/color.hpp"
#include "util/heap_guard.hpp"
#include "util/json_writer.hpp"
#include "util/str.hpp"

namespace lamp {
namespace config_codec {

// Replace a role's segment list from `node["segments"]`. Absent/empty array
// keeps the current (variant-default) list. Every segment is left with ≥1
// color so broadcastColors() is always dereferenceable.
// Fallback: if `segments` is absent/empty but the old flat `colors` array
// is present (blobs from before the per-segment schema), migrate to a single
// segment named `roleName` so custom colors survive the upgrade and the app
// doesn't render a blank segment label.
static void parseSegments(JsonObject node, std::vector<SegmentSettings>& segs,
                          const Color& fallback, const std::string& roleName) {
  JsonArray arr = node["segments"];
  if (arr.isNull() || arr.size() == 0) {
    JsonArray flat = node["colors"];
    if (flat.isNull() || flat.size() == 0) return;
    SegmentSettings seg;
    seg.name = roleName;
    seg.px = node["px"] | 0;
    for (JsonVariant c : flat) {
      if (seg.colors.size() == kMaxSurfaceColors) break;
      seg.colors.push_back(hexStringToColor(c));
    }
    if (seg.colors.empty()) seg.colors.push_back(fallback);
    segs.clear();
    segs.push_back(std::move(seg));
    return;
  }
  segs.clear();
  for (JsonObject segNode : arr) {
    if (segs.size() == kMaxSegments) break;
    SegmentSettings seg;
    seg.name = std::string(segNode["name"] | "");
    truncateUtf8(seg.name, kMaxSegmentNameLen);
    seg.px = segNode["px"] | 0;
    for (JsonVariant c : segNode["colors"].as<JsonArray>()) {
      if (seg.colors.size() == kMaxSurfaceColors) break;
      seg.colors.push_back(hexStringToColor(c));
    }
    if (seg.colors.empty()) seg.colors.push_back(fallback);
    segs.push_back(std::move(seg));
  }
  if (segs.empty()) segs.push_back({roleName, 0, {fallback}});
}

// Σ segment px ≤ 255 (uint8 pixelCount + boot buffer sizing). This is the one
// boundary every NVS load crosses (settings-blob apply AND webapp raw-persist
// both re-parse here on boot), so clamp the running sum: a garbage oversized
// blob can't blow the boot buffer.
static void clampSumPx(std::vector<SegmentSettings>& segs) {
  unsigned budget = 255;
  for (auto& s : segs) {
    if (s.px > budget) s.px = static_cast<uint8_t>(budget);
    budget -= s.px;
  }
}

static void writeSegments(JsonObject node,
                          const std::vector<SegmentSettings>& segs) {
  JsonArray arr = node["segments"].to<JsonArray>();
  for (const auto& s : segs) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = s.name;
    o["px"] = s.px;
    JsonArray colors = o["colors"].to<JsonArray>();
    for (const auto& c : s.colors) colors.add(colorToHexString(c));
  }
}

const ExpressionParams* findExpressionParams(const ExpressionSettings& settings,
                                             const std::string& type, uint8_t target) {
  for (const auto& e : settings.expressions) {
    if (e.target == target && e.type == type) return &e.parameters;
  }
  return nullptr;
}

bool mirrorUpsert(ExpressionSettings& settings, const ExpressionConfig& cfg) {
  auto& exprs = settings.expressions;
  for (auto& e : exprs) {
    if (e.type == cfg.type && e.target == cfg.target) {
      e = cfg;
      return true;
    }
  }
  if (exprs.size() >= kMaxExpressions) return false;
  if (exprs.size() == exprs.capacity()) {
    exprs.reserve(std::min(kMaxExpressions, exprs.capacity() + 4));
  }
  exprs.push_back(cfg);
  return true;
}

std::string storableByteOrder(const char* s) {
  lampos::led::ByteOrder order;
  return s && lampos::led::byteOrderFromString(s, order) ? std::string(s) : std::string();
}

bool parseExpression(JsonObject node, ExpressionConfig& out, bool enabledDefault) {
  bool withinCaps = true;
  out.type = std::string(node["type"] | "");
  if (out.type.size() > kMaxExpressionTypeLen) {
    out.type.resize(kMaxExpressionTypeLen);
    withinCaps = false;
  }
  out.enabled = node["enabled"] | enabledDefault;
  out.intervalMin = node["intervalMin"] | 60;
  out.intervalMax = node["intervalMax"] | 900;
  out.target = node["target"] | 3;
  // disabledDuringWispOverride is a pure type-property, not config-driven.
  // Payloads carrying the key are tolerated; the skip-list drops it.
  for (JsonPair kv : node) {
    const char* key = kv.key().c_str();
    if (!strcmp(key, "type") || !strcmp(key, "enabled") ||
        !strcmp(key, "intervalMin") || !strcmp(key, "intervalMax") ||
        !strcmp(key, "target") || !strcmp(key, "colors") ||
        !strcmp(key, "disabledDuringWispOverride")) {
      continue;
    }
    JsonVariant value = kv.value();
    if (!value.is<uint32_t>() && !value.is<int>()) continue;
    if (expressionParamKeyIndex(key) < 0) continue;
    if (out.parameters.size() >= kMaxExpressionParams) {
      withinCaps = false;
      continue;
    }
    out.setParameter(key, value.is<uint32_t>()
                              ? value.as<uint32_t>()
                              : static_cast<uint32_t>(value.as<int>()));
  }
  JsonArray colors = node["colors"];
  const size_t keep = std::min(colors.size(), kMaxExpressionColors);
  if (colors.size() > keep) withinCaps = false;
  out.colors.reserve(keep);
  for (JsonVariant color : colors) {
    if (out.colors.size() == keep) break;
    out.colors.push_back(hexStringToColor(color));
  }
  return withinCaps;
}

bool fromJson(JsonObject root, LampSettings& lamp, BaseSettings& base,
              ShadeSettings& shade, ExpressionSettings& expressions,
              HomeModeSettings& homeMode) {
  bool withinCaps = true;
  JsonObject lampNode = root["lamp"];
  // lampType is firmware-owned (see Config::setLampType).
  // Inbound settings_blob writes intentionally do not update it; the app
  // can read but not write the variant identity.
  lamp.name =
      normalizeLampName(std::string(lampNode["name"] | kDefaultLampName));
  lamp.brightness = lampNode["brightness"] | 100;
  std::string password = std::string(lampNode["password"] | "");
  if (!password.empty()) {
    // Every write path refuses one past kMaxPasswordLen, so only a store
    // written before the cap holds a longer one; holding the BOOT button at
    // power-up clears a password the owner can no longer type.
    truncateUtf8(password, kMaxPasswordLen);
    lamp.password = password;
  }
  lamp.setup = lampNode["setup"] | false;
  lamp.named = lampNode["named"] | false;
  lamp.advancedEnabled = lampNode["advancedEnabled"] | false;
  lamp.webappEnabled = lampNode["webappEnabled"] | true;
  lamp.apBootMinutes = lampNode["apBootMinutes"] | 0;
  lamp.brightnessCeiling = lampNode["brightnessCeiling"] | 170;
  // SocialMode persists as uint8_t (0=Introvert, 1=Ambivert, 2=Extrovert).
  // Out-of-range values fall back to Ambivert so a corrupt or future-versioned
  // payload doesn't strand the lamp in an unknown personality.
  {
    uint32_t modeRaw = lampNode["socialMode"] | 1;
    if (modeRaw > 2) modeRaw = 1;
    lamp.socialMode = static_cast<SocialMode>(modeRaw);
  }

  JsonObject baseNode = root["base"];
  base.byteOrder = storableByteOrder(baseNode["byteOrder"] | "");
  parseSegments(baseNode, base.segments, kBaseDefaultColor, "Base");
  clampSumPx(base.segments);
  // Keep knockoutPixels in sync with the active pixel count. Drops stale
  // entries when px shrinks and 100-fills ("no knockout") any slots when px
  // grows. The input loop below then overwrites slots 0..sumPx-1 from JSON.
  base.knockoutPixels.resize(base.sumPx(), 100);
  // Knockout: positional uint8 array, one entry per pixel. Index = pixel;
  // value = brightness % (0..100, default 100). Matches `asBaseJson` /
  // `asJsonDocument` emit shape.
  JsonArray baseKnockoutPixels = baseNode["knockout"];
  for (size_t i = 0;
       i < baseKnockoutPixels.size() && i < base.knockoutPixels.size();
       i++) {
    int value = baseKnockoutPixels[i] | 100;
    base.knockoutPixels[i] = (uint8_t)value;
  }

  JsonObject shadeNode = root["shade"];
  shade.byteOrder = storableByteOrder(shadeNode["byteOrder"] | "");
  parseSegments(shadeNode, shade.segments, kShadeDefaultColor, "Shade");
  clampSumPx(shade.segments);
  // colorsEditable is firmware-owned (set by Lamp subclass defaults via
  // applyDefaults). Inbound writes intentionally do not update it.

  // Load expressions
  JsonArray expressionsNode = root["expressions"];
  if (expressionsNode) {
    expressions.expressions.clear();
    expressions.expressions.reserve(
        std::min(expressionsNode.size(), kMaxExpressions));
    for (JsonObject exprNode : expressionsNode) {
      if (expressions.expressions.size() == kMaxExpressions) {
        withinCaps = false;
        break;
      }
      ExpressionConfig expr;
      if (!parseExpression(exprNode, expr, false)) withinCaps = false;
      expressions.expressions.push_back(std::move(expr));
    }
  }

  // Load home mode. A legacy "password" field in NVS is silently ignored
  // (presence-only home mode stores no password).
  JsonObject homeModeNode = root["homeMode"];
  if (homeModeNode) {
    homeMode.ssid = std::string(homeModeNode["ssid"] | "");
    truncateUtf8(homeMode.ssid, kMaxSsidLen);
    homeMode.brightness = homeModeNode["brightness"] | 60;
    // Migration: lamps configured before `enabled` existed have no
    // "enabled" key in their NVS-stored JSON. Treat "has SSID" as proxy
    // for "user wanted home mode on" so they keep working post-update.
    homeMode.enabled = homeModeNode["enabled"] | !homeMode.ssid.empty();
    homeMode.networkBound = homeModeNode["networkBound"] | !homeMode.ssid.empty();
    homeMode.socialDisabled = homeModeNode["socialDisabled"] | true;
    if (homeModeNode["disabledExpressionTypes"].is<JsonArray>()) {
      homeMode.disabledExpressionTypes.clear();
      for (JsonVariant v : homeModeNode["disabledExpressionTypes"].as<JsonArray>()) {
        if (homeMode.disabledExpressionTypes.size() == kMaxDisabledExpressionTypes) break;
        const char* s = v.as<const char*>();
        if (isExpressionTypeId(s)) homeMode.disabledExpressionTypes.push_back(s);
      }
    } else {
      homeMode.disabledExpressionTypes = {"glitchy"};
    }
  }
  return withinCaps;
}

void toJson(JsonObject root, const LampSettings& lamp, const BaseSettings& base,
            const ShadeSettings& shade, const ExpressionSettings& expressions,
            const HomeModeSettings& homeMode) {
  toStoredJson(root, lamp, base, shade, homeMode);
  // Positional uint8 array, same shape as asBaseJson.
  JsonArray knockoutNode = root["base"]["knockout"].to<JsonArray>();
  for (uint8_t level : base.knockoutPixels) knockoutNode.add(level);
  JsonArray expressionsNode = root["expressions"].to<JsonArray>();
  for (const auto& expr : expressions.expressions) {
    expressionToJson(expressionsNode.add<JsonObject>(), expr);
  }
}

void writeExpressionJson(JsonWriter& w, const ExpressionConfig& expr) {
  w.beginObject();
  w.key("type");
  w.value(expr.type.c_str());
  w.key("enabled");
  w.value(expr.enabled);
  w.key("intervalMin");
  w.value(expr.intervalMin);
  w.key("intervalMax");
  w.value(expr.intervalMax);
  w.key("target");
  w.value(static_cast<uint32_t>(expr.target));
  for (const auto& param : expr.parameters) {
    w.key(param.first);
    w.value(param.second);
  }
  w.key("colors");
  w.beginArray();
  for (const auto& color : expr.colors) w.value(colorToHexString(color).c_str());
  w.endArray();
  w.endObject();
}

static void writeSegmentsJson(JsonWriter& w, const std::vector<SegmentSettings>& segs) {
  w.key("segments");
  w.beginArray();
  for (const auto& seg : segs) {
    w.beginObject();
    w.key("name");
    w.value(seg.name.c_str());
    w.key("px");
    w.value(static_cast<uint32_t>(seg.px));
    w.key("colors");
    w.beginArray();
    for (const auto& c : seg.colors) w.value(colorToHexString(c).c_str());
    w.endArray();
    w.endObject();
  }
  w.endArray();
}

void writeStoredJson(JsonWriter& w, const LampSettings& lamp, const BaseSettings& base,
                     const ShadeSettings& shade, const HomeModeSettings& homeMode,
                     const ExpressionSettings* whole) {
  w.beginObject();
  w.key("lamp");
  w.beginObject();
  w.key("name");
  w.value(lamp.name.c_str());
  w.key("brightness");
  w.value(static_cast<uint32_t>(lamp.brightness));
  if (!lamp.password.empty()) {
    w.key("password");
    w.value(lamp.password.c_str());
  }
  w.key("setup");
  w.value(lamp.setup);
  w.key("named");
  w.value(lamp.named);
  w.key("advancedEnabled");
  w.value(lamp.advancedEnabled);
  w.key("webappEnabled");
  w.value(lamp.webappEnabled);
  w.key("apBootMinutes");
  w.value(static_cast<uint32_t>(lamp.apBootMinutes));
  w.key("brightnessCeiling");
  w.value(static_cast<uint32_t>(lamp.brightnessCeiling));
  w.key("socialMode");
  w.value(static_cast<uint32_t>(lamp.socialMode));
  w.endObject();
  w.key("base");
  w.beginObject();
  if (!base.byteOrder.empty()) {
    w.key("byteOrder");
    w.value(base.byteOrder.c_str());
  }
  w.key("colorsEditable");
  w.value(base.colorsEditable);
  writeSegmentsJson(w, base.segments);
  if (whole) {
    w.key("knockout");
    w.beginArray();
    for (uint8_t level : base.knockoutPixels) w.value(static_cast<uint32_t>(level));
    w.endArray();
  }
  w.endObject();
  w.key("shade");
  w.beginObject();
  if (!shade.byteOrder.empty()) {
    w.key("byteOrder");
    w.value(shade.byteOrder.c_str());
  }
  w.key("colorsEditable");
  w.value(shade.colorsEditable);
  writeSegmentsJson(w, shade.segments);
  w.endObject();
  w.key("homeMode");
  w.beginObject();
  w.key("ssid");
  w.value(homeMode.ssid.c_str());
  w.key("brightness");
  w.value(static_cast<uint32_t>(homeMode.brightness));
  w.key("enabled");
  w.value(homeMode.enabled);
  w.key("networkBound");
  w.value(homeMode.networkBound);
  w.key("socialDisabled");
  w.value(homeMode.socialDisabled);
  w.key("disabledExpressionTypes");
  w.beginArray();
  for (const auto& t : homeMode.disabledExpressionTypes) w.value(t.c_str());
  w.endArray();
  w.endObject();
  if (whole) {
    w.key("expressions");
    w.beginArray();
    for (const auto& expr : whole->expressions) writeExpressionJson(w, expr);
    w.endArray();
  }
  w.endObject();
}

void expressionToJson(JsonObject node, const ExpressionConfig& expr) {
  node["type"] = expr.type;
  node["enabled"] = expr.enabled;
  node["intervalMin"] = expr.intervalMin;
  node["intervalMax"] = expr.intervalMax;
  node["target"] = expr.target;
  for (const auto& param : expr.parameters) node[param.first] = param.second;
  JsonArray colorsNode = node["colors"].to<JsonArray>();
  for (const auto& color : expr.colors) colorsNode.add(colorToHexString(color));
}

void toStoredJson(JsonObject root, const LampSettings& lamp,
                  const BaseSettings& base, const ShadeSettings& shade,
                  const HomeModeSettings& homeMode) {
  JsonObject lampNode = root["lamp"].to<JsonObject>();
  lampNode["name"] = lamp.name;
  lampNode["brightness"] = lamp.brightness;
  if (!lamp.password.empty()) {
    lampNode["password"] = lamp.password;
  }
  lampNode["setup"] = lamp.setup;
  lampNode["named"] = lamp.named;
  lampNode["advancedEnabled"] = lamp.advancedEnabled;
  lampNode["webappEnabled"] = lamp.webappEnabled;
  lampNode["apBootMinutes"] = lamp.apBootMinutes;
  lampNode["brightnessCeiling"] = lamp.brightnessCeiling;
  lampNode["socialMode"] = static_cast<uint8_t>(lamp.socialMode);
  JsonObject baseNode = root["base"].to<JsonObject>();
  if (!base.byteOrder.empty()) baseNode["byteOrder"] = base.byteOrder;
  baseNode["colorsEditable"] = base.colorsEditable;
  writeSegments(baseNode, base.segments);

  JsonObject shadeNode = root["shade"].to<JsonObject>();
  if (!shade.byteOrder.empty()) shadeNode["byteOrder"] = shade.byteOrder;
  shadeNode["colorsEditable"] = shade.colorsEditable;
  writeSegments(shadeNode, shade.segments);

  JsonObject homeModeNode = root["homeMode"].to<JsonObject>();
  homeModeNode["ssid"] = homeMode.ssid;
  homeModeNode["brightness"] = homeMode.brightness;
  homeModeNode["enabled"] = homeMode.enabled;
  homeModeNode["networkBound"] = homeMode.networkBound;
  homeModeNode["socialDisabled"] = homeMode.socialDisabled;
  JsonArray disArr = homeModeNode["disabledExpressionTypes"].to<JsonArray>();
  for (const auto& s : homeMode.disabledExpressionTypes) disArr.add(s);
}

namespace {

constexpr uint8_t kRecordFormat = 1;
constexpr uint8_t kRecordEnabled = 0x01;
constexpr const char* kCountKey = "xn";
constexpr const char* kKnockoutKey = "ko";
// Record keys any build has written ("x0".."x31"); the cap has been lower.
constexpr size_t kRecordKeySlots = 32;

void putU32(uint8_t*& p, uint32_t v) {
  for (int i = 0; i < 4; ++i) *p++ = static_cast<uint8_t>(v >> (8 * i));
}

uint32_t getU32(const uint8_t*& p) {
  uint32_t v = 0;
  for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(*p++) << (8 * i);
  return v;
}

void recordKey(size_t i, char (&key)[5]) {
  std::snprintf(key, sizeof(key), "x%u", static_cast<unsigned>(i));
}


}  // namespace

size_t encodeExpression(const ExpressionConfig& expr, uint8_t* out) {
  uint8_t* p = out;
  *p++ = kRecordFormat;
  *p++ = expr.enabled ? kRecordEnabled : 0;
  *p++ = expr.target;
  const size_t typeLen = std::min(expr.type.size(), kMaxExpressionTypeLen);
  *p++ = static_cast<uint8_t>(typeLen);
  std::memcpy(p, expr.type.data(), typeLen);
  p += typeLen;
  putU32(p, expr.intervalMin);
  putU32(p, expr.intervalMax);
  const size_t colors = std::min(expr.colors.size(), kMaxExpressionColors);
  *p++ = static_cast<uint8_t>(colors);
  for (size_t i = 0; i < colors; ++i) {
    const Color& c = expr.colors[i];
    *p++ = c.r;
    *p++ = c.g;
    *p++ = c.b;
    *p++ = c.w;
  }
  uint8_t* paramCount = p++;
  *paramCount = 0;
  for (const auto& kv : expr.parameters) {
    if (*paramCount == kMaxExpressionParams) break;
    const int idx = expressionParamKeyIndex(kv.first);
    if (idx < 0) continue;
    *p++ = static_cast<uint8_t>(idx);
    putU32(p, kv.second);
    ++*paramCount;
  }
  return static_cast<size_t>(p - out);
}

bool decodeExpression(const uint8_t* data, size_t len, ExpressionConfig& out) {
  const uint8_t* p = data;
  const uint8_t* end = data + len;
  auto have = [&](size_t n) { return static_cast<size_t>(end - p) >= n; };
  if (!have(4) || p[0] != kRecordFormat) return false;
  out.enabled = (p[1] & kRecordEnabled) != 0;
  out.target = p[2];
  const size_t typeLen = p[3];
  p += 4;
  if (typeLen > kMaxExpressionTypeLen || !have(typeLen + 8 + 1)) return false;
  out.type.assign(reinterpret_cast<const char*>(p), typeLen);
  p += typeLen;
  out.intervalMin = getU32(p);
  out.intervalMax = getU32(p);
  const size_t colors = *p++;
  if (colors > kMaxExpressionColors || !have(4 * colors + 1)) return false;
  out.colors.clear();
  out.colors.reserve(colors);
  for (size_t i = 0; i < colors; ++i, p += 4) {
    out.colors.push_back(Color(p[0], p[1], p[2], p[3]));
  }
  const size_t params = *p++;
  if (params > kMaxExpressionParams || !have(5 * params)) return false;
  out.parameters.clear();
  for (size_t i = 0; i < params; ++i) {
    const uint8_t idx = *p++;
    const uint32_t value = getU32(p);
    // An index past this build's table came from a newer one; skip it.
    if (idx < kExpressionParamKeyCount) {
      out.parameters.setIndex(idx, value);
    }
  }
  return true;
}

SaveResult saveConfig(ConfigStore& store, const LampSettings& lamp,
                      const BaseSettings& base, const ShadeSettings& shade,
                      const ExpressionSettings& expressions,
                      const HomeModeSettings& homeMode, char* cfgBuffer,
                      size_t cfgBufferLen) {
  char key[5];

  // Whole: everything in the one "cfg" JSON, the form every firmware reads,
  // so a lamp flashed back to an older build keeps its expressions. The
  // split keys from an earlier save go after it lands; until then a load
  // prefers the expressions and knockout the cfg JSON carries.
  {
    JsonWriter whole(cfgBuffer, cfgBufferLen);
    writeStoredJson(whole, lamp, base, shade, homeMode, &expressions);
    whole.finish();
    if (whole.ok()) {
      if (store.write("cfg", cfgBuffer) == 0) return SaveResult::WriteFailed;
      // Every record slot any build wrote, whatever the count says: a split
      // save cut short before its count landed leaves records the count
      // never named. Records go before the count, so no reset can leave a
      // record no later save reaches.
      for (size_t i = 0; i < kRecordKeySlots; ++i) {
        recordKey(i, key);
        store.erase(key);
      }
      store.erase(kCountKey);
      store.erase(kKnockoutKey);
      return SaveResult::Ok;
    }
  }

  // Split: too big for one page-sized string. The cfg JSON is built first,
  // so a TooLarge refusal writes nothing.
  JsonWriter rest(cfgBuffer, cfgBufferLen);
  writeStoredJson(rest, lamp, base, shade, homeMode, nullptr);
  rest.finish();
  if (!rest.ok()) return SaveResult::TooLarge;

  const size_t count = std::min(expressions.expressions.size(), kMaxExpressions);
  uint8_t record[kMaxExpressionRecordLen];
  for (size_t i = 0; i < count; ++i) {
    const size_t len = encodeExpression(expressions.expressions[i], record);
    recordKey(i, key);
    if (store.writeBlob(key, record, len) == 0) return SaveResult::WriteFailed;
  }
  if (!base.knockoutPixels.empty() &&
      store.writeBlob(kKnockoutKey, base.knockoutPixels.data(),
                      base.knockoutPixels.size()) == 0) {
    return SaveResult::WriteFailed;
  }
  const uint8_t n = static_cast<uint8_t>(count);
  if (store.writeBlob(kCountKey, &n, 1) == 0) return SaveResult::WriteFailed;
  // Every slot past the count, so a record left by a save cut short is gone.
  for (size_t i = count; i < kRecordKeySlots; ++i) {
    recordKey(i, key);
    store.erase(key);
  }
  return store.write("cfg", cfgBuffer) > 0 ? SaveResult::Ok : SaveResult::WriteFailed;
}

LoadedRecords loadRecords(ConfigStore& store, BaseSettings& base,
                          ExpressionSettings& expressions, bool cfgHasKnockout,
                          bool cfgHasExpressions) {
  LoadedRecords result;
  if (!cfgHasKnockout) {
    uint8_t knockout[255];
    const size_t knockoutLen = store.readBlob(kKnockoutKey, knockout, sizeof(knockout));
    for (size_t i = 0; i < knockoutLen && i < base.knockoutPixels.size(); ++i) {
      base.knockoutPixels[i] = knockout[i];
    }
  }
  if (cfgHasExpressions) return result;
  uint8_t n = 0;
  if (store.readBlob(kCountKey, &n, 1) != 1) return result;
  result.present = true;
  expressions.expressions.clear();
  expressions.expressions.reserve(std::min<size_t>(n, kMaxExpressions));
  uint8_t record[kMaxExpressionRecordLen];
  char key[5];
  for (size_t i = 0; i < n && i < kMaxExpressions; ++i) {
    recordKey(i, key);
    const size_t len = store.readBlob(key, record, sizeof(record));
    ExpressionConfig expr;
    if (len == 0 || !decodeExpression(record, len, expr)) {
      result.droppedMask |= 1u << i;
      continue;
    }
    // A save interrupted mid-shift (a remove moves later records down one
    // slot) leaves the moved record in two slots; the first is the live one.
    const bool duplicate = std::any_of(
        expressions.expressions.begin(), expressions.expressions.end(),
        [&](const ExpressionConfig& e) {
          return e.type == expr.type && e.target == expr.target;
        });
    if (duplicate) {
      ++result.duplicates;
      continue;
    }
    expressions.expressions.push_back(std::move(expr));
  }
  return result;
}

bool configBlobPersistable(const char* json) {
  if (json == nullptr) return false;
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
  return doc["lamp"].is<JsonObject>();
}

}  // namespace config_codec
}  // namespace lamp
