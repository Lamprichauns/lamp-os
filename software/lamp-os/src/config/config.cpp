#include "config.hpp"

#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>
#include <esp_heap_caps.h>
#include <new>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config/config_codec.hpp"
#include "core/crash_record.hpp"
#include "core/lamp.hpp"
#include "expr_catalog.h"
#include "util/bd_addr.hpp"
#include "util/color.hpp"
#include "util/heap_guard.hpp"
#include "util/json_writer.hpp"
#include "version.hpp"

#include <lampos/protocol/presence.hpp>

namespace lamp {

// Seed every segment of a role from the variant defaults, but only while NVS
// still holds the single class-default segment. A configured lamp (NVS parsed
// its own segment list) keeps its saved segments untouched.
static void seedRoleSegments(std::vector<SegmentSettings>& segs,
                             const std::vector<Config::SegmentDefault>& seeds,
                             const Color& factory) {
  const bool factoryUnconfigured =
      segs.size() == 1 && segs[0].colors.size() == 1 && segs[0].colors[0] == factory;
  if (!factoryUnconfigured) return;
  segs.clear();
  for (const auto& s : seeds) {
    segs.push_back({s.name, s.px, parseColorList(s.colors)});
  }
}

Config::Config(ConfigStore* inStore) {
  JsonDocument doc;
  store_ = inStore;
  dispositions_.attachStore(store_);
  std::string json = store_->read("cfg", "{}");
  DeserializationError error = deserializeJson(doc, json);

#ifdef LAMP_DEBUG
  // Print a compact, secret-free summary instead of the raw NVS JSON.
  // The raw payload would leak `lamp.password` to anyone on the serial
  // console (USB physical access). If you need the full shape while
  // debugging a parse bug, attach a temporary `Serial.println(json)`
  // for that session; don't roll back this redaction. Reads off `doc`
  // (already parsed above) rather than a second JsonDocument + parse.
  if (error == DeserializationError::Ok) {
    const char* loadedName = doc["lamp"]["name"] | "<unset>";
    const char* pwField = doc["lamp"]["password"] | "";
    const bool hasPassword = pwField != nullptr && pwField[0] != '\0';
    Serial.printf("[cfg] loaded name=%s pw=%s cfg_bytes=%u\n", loadedName,
                  hasPassword ? "set" : "unset", (unsigned)json.length());
  } else {
    Serial.printf("[cfg] loaded nvs_bytes=%u (parse failed; full dump suppressed)\n",
                  (unsigned)json.length());
  }
#endif

  if (error) {
    if (configBlobHasData(json)) {
      loadFailedWithData_ = true;
      Serial.printf(
          "[cfg] STORED CONFIG FAILED TO PARSE (%u bytes) — serving defaults "
          "WITHOUT overwriting NVS\n",
          (unsigned)json.length());
    }
#ifdef LAMP_DEBUG
    Serial.printf("ws deserializeJson() failed: %s\n", error.c_str());
#endif
    return;  // use class defaults
  }

  bool withinCaps = true;
  const config_codec::LoadedRecords records = config_codec::loadStoredConfig(
      *store_, doc.as<JsonObject>(), lamp, base, shade, expressions, homeMode, withinCaps);
  if (!withinCaps) Serial.println("[cfg] stored expressions over caps; truncated on load");
  for (size_t i = 0; i < kMaxExpressions; ++i) {
    if (records.droppedMask & (1u << i)) {
      Serial.printf("[cfg] expression record x%u unreadable; dropped\n", (unsigned)i);
    }
  }
  if (records.duplicates) {
    Serial.printf("[cfg] %u repeated expression records skipped\n",
                  (unsigned)records.duplicates);
  }
  Serial.printf("[cfg] expressions=%u from %s\n",
                (unsigned)expressions.expressions.size(),
                records.present ? "records" : "cfg json");
  namedKeyPresent_ = doc["lamp"]["named"].is<bool>();

  // Per-peer dispositions live in a separate NVS key.
  dispositions_.load();
};

// The "cfg" JSON is written into this reserved buffer, so a save never
// waits for a large free block. Loop task only: every save, the webapp's
// included (webapp::tick drains it), runs there.
static char s_cfgJson[config_codec::kMaxStoredCfgJsonLen + 1];

bool Config::persistConfig(const char* via) {
  if (!store_) return false;
  if (savesFrozen_) {
    Serial.printf("[nvs] config save via=%s skipped: a web save awaits its reboot\n", via);
    return false;
  }
  const uint32_t now = millis();
  const config_codec::SaveResult result = config_codec::saveConfig(
      *store_, lamp, base, shade, expressions, homeMode, s_cfgJson, sizeof(s_cfgJson));
  if (result == config_codec::SaveResult::Ok) {
    persistRetry_.succeed();
#ifdef LAMP_DEBUG
    Serial.printf("[nvs] config saved via=%s\n", via);
#endif
    return true;
  }
  persistRetry_.fail(now);
  Serial.printf("[nvs] config save via=%s failed (%s); retry in %lus\n", via,
                result == config_codec::SaveResult::TooLarge ? "cfg json too large" : "write",
                (unsigned long)(persistRetry_.waitMs() / 1000));
  return false;
}

bool Config::persistDue(uint32_t nowMs) const { return persistRetry_.due(nowMs); }

void Config::maybeRetryPersist(uint32_t nowMs) {
  if (persistRetry_.pending() && persistRetry_.due(nowMs)) persistConfig("retry");
}

bool Config::persistRawJson(const char* json) {
  if (!store_) return false;
  if (!config_codec::configBlobPersistable(json)) {
#ifdef LAMP_DEBUG
    Serial.println("[nvs] persistRawJson rejected: blob did not validate; NVS left intact");
#endif
    return false;
  }
  // Parse into a scratch model and save it in the stored shape; the caller
  // reboots to load it, the same as a raw write re-parsed on the next boot.
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
  LampSettings rawLamp;
  BaseSettings rawBase;
  ShadeSettings rawShade;
  ExpressionSettings rawExpressions;
  HomeModeSettings rawHome;
  config_codec::fromJson(doc.as<JsonObject>(), rawLamp, rawBase, rawShade,
                         rawExpressions, rawHome);
  if (config_codec::saveConfig(*store_, rawLamp, rawBase, rawShade, rawExpressions,
                               rawHome, s_cfgJson, sizeof(s_cfgJson)) !=
      config_codec::SaveResult::Ok) {
    return false;
  }
  // The stored config is now the web document, which the reboot loads; a
  // pending retry or a later in-memory save would overwrite it with the
  // running config first.
  persistRetry_.succeed();
  savesFrozen_ = true;
  return true;
}

bool Config::factoryReset() { return store_ ? store_->clear() : false; }

void Config::setLampType(const std::string& type) {
  lamp.lampType = type;
  if (!store_) return;
  store_->write("lampType", type.c_str());
}

std::string Config::loadLampType() {
  lamp.lampType = "";
  if (!store_) return "";
  lamp.lampType = store_->read("lampType", "");
  return lamp.lampType;
}

void Config::setLampId(const std::string& lampId) {
  if (lampId == lampId_) return;
  lampId_ = lampId;
  invalidateLampSection();
}

void Config::setLampOtaState(uint8_t s, const uint8_t* targetMac) {
  const bool hasTarget = targetMac != nullptr;
  if (s == lampOtaState_ && hasTarget == lampHasOtaSendingTo_ &&
      (!hasTarget || std::memcmp(targetMac, lampOtaSendingTo_, 6) == 0)) {
    return;
  }
  lampOtaState_ = s;
  lampHasOtaSendingTo_ = hasTarget;
  if (hasTarget) std::memcpy(lampOtaSendingTo_, targetMac, 6);
  invalidateLampSection();
}

bool Config::isOtaDistributing() const {
  return lampOtaState_ == lamp_protocol::kOtaStateSending;
}

uint8_t Config::getDisposition(const uint8_t mac[6]) const {
  return dispositions_.get(mac);
}

uint8_t Config::getDisposition(const std::string& lampId) const {
  uint8_t mac[6];
  if (!parseBdAddr(lampId.c_str(), mac)) return DispositionStore::kDefault;
  return dispositions_.get(mac);
}

void Config::setDisposition(const std::string& lampId, uint8_t value) {
  uint8_t mac[6];
  if (!parseBdAddr(lampId.c_str(), mac)) return;
  dispositions_.set(mac, value, millis());
}

std::string Config::asDispositionsJson() const { return dispositions_.asJson(); }

bool Config::setDispositionsFromJson(const char* json, size_t len) {
  return dispositions_.setFromJson(json, len, millis());
}

void Config::maybeFlushDispositions(uint32_t nowMs) {
  dispositions_.maybeFlush(nowMs);
}

void Config::flushDispositionsNow() { dispositions_.flushNow(millis()); }

JsonDocument Config::asJsonDocument() {
  JsonDocument doc;
  config_codec::toJson(doc.to<JsonObject>(), lamp, base, shade, expressions,
                       homeMode);
  return doc;
};

static void emitLastCrash(JsonDocument& doc, const crash_record::Record& rec) {
  if (rec.count == 0) return;
  JsonObject o = doc["lastCrash"].to<JsonObject>();
  o["count"] = rec.count;
  o["reset"] = rec.resetReason;
  o["fw"] = rec.fwVersion;
  if (!rec.snapshotValid) return;
  const crash_record::Snapshot& s = rec.snap;
  o["uptimeMs"] = s.uptimeMs;
  o["minLargestBlock"] = s.minLargestBlock;
  o["minFreeHeap"] = s.minFreeHeap;
  o["rosterPeak"] = s.rosterPeak;
  if (s.core < 0) return;
  char hex[11];
  o["reason"] = static_cast<const char*>(s.reason);
  o["core"] = s.core;
  std::snprintf(hex, sizeof(hex), "0x%08x", static_cast<unsigned>(s.pc));
  o["pc"] = hex;
  JsonArray bt = o["bt"].to<JsonArray>();
  for (uint32_t addr : s.backtrace) {
    if (addr == 0) break;
    std::snprintf(hex, sizeof(hex), "0x%08x", static_cast<unsigned>(addr));
    bt.add(hex);
  }
}

JsonDocument Config::asLampJson() {
  JsonDocument doc;
  doc["name"] = lamp.name;
  doc["brightness"] = lamp.brightness;
  if (!lamp.password.empty()) {
    doc["password"] = lamp.password;
  }
  // Authoritative password-set state. Lets the app detect divergence
  // (its cached controlPassword is non-empty but NVS pw was wiped) and
  // auto-clear its cache so subsequent settings_blob writes go through
  // the plaintext branch instead of failing decrypt silently.
  doc["hasPassword"] = !lamp.password.empty();
  doc["advancedEnabled"] = lamp.advancedEnabled;
  doc["webappEnabled"] = lamp.webappEnabled;
  doc["apBootMinutes"] = lamp.apBootMinutes;
  doc["brightnessCeiling"] = lamp.brightnessCeiling;
  doc["socialMode"] = static_cast<uint8_t>(lamp.socialMode);
  // Firmware identity (packed semver + release channel string). Constant at
  // boot, so no extra invalidation hook is needed; the lamp section cache
  // picks these up the first time it's built.
  doc["fwVersion"] = FIRMWARE_VERSION;
  doc["fwChannel"] = FIRMWARE_CHANNEL_STR;
  // Content address for the flash expr catalog, so the app fetches a given
  // catalog at most once. Hash precomputed at build time over the exact bytes
  // served by the exprcat section.
  char catalogHash[9];
  std::snprintf(catalogHash, sizeof(catalogHash), "%08x", lamp::kExprCatalogHash);
  doc["catalogHash"] = catalogHash;
  // lampType is firmware-owned; the app can read it but settings_blob
  // writes do NOT update it (see parser block below).
  doc["lampType"] = lamp.lampType;
  if (!lampId_.empty()) {
    doc["lampId"] = lampId_;
  }
  if (lampOtaState_ != 0) doc["otaState"] = lampOtaState_;
  if (lampHasOtaSendingTo_) {
    char macBuf[18];
    formatBdAddr(lampOtaSendingTo_, macBuf);
    doc["otaSendingTo"] = macBuf;
  }
  doc["resetReason"] = static_cast<uint8_t>(esp_reset_reason());
  emitLastCrash(doc, crash_record::last());
  return doc;
}

// Role sections advertise topology: a role-level `px` = Σ segment px (the
// app's scalar readers size off it) alongside the per-segment list. Both are
// emitted; the app reads whichever it needs.
static void emitRoleTopology(JsonDocument& doc,
                             const std::vector<SegmentSettings>& segs) {
  doc["px"] = segmentsSumPx(segs);
  JsonArray colorsNode = doc["colors"].to<JsonArray>();
  for (const auto& c : segs.front().colors) colorsNode.add(colorToHexString(c));
  JsonArray segsNode = doc["segments"].to<JsonArray>();
  for (const auto& s : segs) {
    JsonObject o = segsNode.add<JsonObject>();
    o["name"] = s.name;
    o["px"] = s.px;
    JsonArray sc = o["colors"].to<JsonArray>();
    for (const auto& c : s.colors) sc.add(colorToHexString(c));
  }
}

JsonDocument Config::asBaseJson() {
  JsonDocument doc;
  emitRoleTopology(doc, base.segments);
  doc["byteOrder"] = base.byteOrder;
  // Knockout: positional uint8 array, one entry per pixel.
  //   wire shape: "knockout":[100,100,6,9,...] (length = knockoutPixels.size())
  // Index = pixel; value = brightness % (0..100). Caps the base section at
  // ~200 bytes regardless of pattern density so a dense knockout fits
  // under the per-characteristic ATT cap.
  JsonArray knockoutNode = doc["knockout"].to<JsonArray>();
  for (size_t i = 0; i < base.knockoutPixels.size(); i++) {
    knockoutNode.add((int)base.knockoutPixels[i]);
  }
  // colorsEditable is firmware-owned (set by Lamp subclass via applyDefaults).
  // Emitted so the app can hide the color picker for surfaces the subclass
  // has opted out of (e.g. SnafuLamp shade).
  doc["colorsEditable"] = base.colorsEditable;
  doc["drawIdleMa"] = drawIdleMa_;
  doc["drawFullMa"] = drawFullMa_;
  return doc;
}

JsonDocument Config::asShadeJson() {
  JsonDocument doc;
  emitRoleTopology(doc, shade.segments);
  doc["byteOrder"] = shade.byteOrder;
  // colorsEditable is firmware-owned (set by Lamp subclass via applyDefaults).
  // Emitted so the app can hide the color picker for surfaces the subclass
  // has opted out of (e.g. SnafuLamp shade).
  doc["colorsEditable"] = shade.colorsEditable;
  return doc;
}

JsonDocument Config::asHomeModeJson() {
  JsonDocument doc;
  doc["ssid"] = homeMode.ssid;
  doc["brightness"] = homeMode.brightness;
  doc["enabled"] = homeMode.enabled;
  doc["networkBound"] = homeMode.networkBound;
  doc["socialDisabled"] = homeMode.socialDisabled;
  JsonArray disArr = doc["disabledExpressionTypes"].to<JsonArray>();
  for (const auto& s : homeMode.disabledExpressionTypes) disArr.add(s);
  return doc;
}

// Per-section JSON cache accessors. Rebuild-if-dirty and the reader's
// copy-out share one mutex; without it a reader on one core can copy the
// member string while a rebuild on the other core reallocates it.
static SemaphoreHandle_t cacheMutex() {
  static SemaphoreHandle_t m = xSemaphoreCreateMutex();
  return m;
}

// The heap check before each of these cannot reserve the block: another
// task can take it first. The catch is the last line, best effort since the
// build has no exception emergency pool.
static bool assignNoThrow(std::string& dst, const char* src, size_t len) {
  try {
    dst.assign(src, len);
    return true;
  } catch (const std::bad_alloc&) {
    return false;
  }
}

static bool resizeNoThrow(std::string& dst, size_t len) {
  try {
    dst.resize(len);
    return true;
  } catch (const std::bad_alloc&) {
    return false;
  }
}

// A field OOM must stay visible without spamming: warn on the degrade path
// on every channel, throttled to one line per LAMP_SECTION_OOM_MIN_MS.
static void logSectionOom(const char* msg) {
  static constexpr uint32_t LAMP_SECTION_OOM_MIN_MS = 5000;
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < LAMP_SECTION_OOM_MIN_MS && lastMs != 0) return;
  lastMs = now;
  Serial.printf("[cfg] %s section OOM; serving stale\n", msg);
}

// A web/BLE section read must never panic the lamp or serve a truncated
// section. The rebuild serializes into one guarded buffer (an Arduino String
// grown on a fragmented heap silently stops appending and would cache a cut-off
// JSON as valid). On failure keep the last-good cache, leave the dirty flag
// set so it retries when heap recovers, and serve the stale (or empty) bytes.
#define LAMP_DEFINE_SECTION_CACHED(NAME, BUILDER)                          \
  void Config::NAME##SectionRebuildIfDirty() {                             \
    xSemaphoreTake(cacheMutex(), portMAX_DELAY);                           \
    if (NAME##SectionDirty_) {                                             \
      size_t len = 0;                                                      \
      std::unique_ptr<char[]> json;                                        \
      if (heapCanSpare(kHeapHeadroom)) {                                   \
        JsonDocument doc = BUILDER();                                      \
        json = serializeGuarded(doc, len);                                 \
      }                                                                    \
      if (json && (NAME##SectionJson_.capacity() >= len ||                 \
                   heapCanSpare(len)) &&                                   \
          assignNoThrow(NAME##SectionJson_, json.get(), len)) {            \
        NAME##SectionDirty_ = false;                                       \
      } else {                                                             \
        logSectionOom(#NAME);                                              \
      }                                                                    \
    }                                                                      \
    xSemaphoreGive(cacheMutex());                                          \
  }                                                                        \
  void Config::NAME##SectionJsonCached(std::string& out) {                 \
    NAME##SectionRebuildIfDirty();                                         \
    xSemaphoreTake(cacheMutex(), portMAX_DELAY);                           \
    if (!((out.capacity() >= NAME##SectionJson_.size() ||                  \
           heapCanSpare(NAME##SectionJson_.size())) &&                     \
          assignNoThrow(out, NAME##SectionJson_.data(),                    \
                        NAME##SectionJson_.size()))) {                     \
      out.clear();                                                         \
      logSectionOom(#NAME);                                                \
    }                                                                      \
    xSemaphoreGive(cacheMutex());                                          \
  }

LAMP_DEFINE_SECTION_CACHED(lamp, asLampJson)
LAMP_DEFINE_SECTION_CACHED(base, asBaseJson)
LAMP_DEFINE_SECTION_CACHED(shade, asShadeJson)
LAMP_DEFINE_SECTION_CACHED(home, asHomeModeJson)

// The largest section (about 4 KB at the caps) is written straight into its
// cache, measured first, so a rebuild takes one exactly-sized block and never
// a document's worth of small ones.
static void writeExpressionsSection(JsonWriter& w, const ExpressionSettings& s) {
  w.beginArray();
  for (const auto& expr : s.expressions) config_codec::writeExpressionJson(w, expr);
  w.endArray();
  w.finish();
}

void Config::expressionsSectionRebuildIfDirty() {
  xSemaphoreTake(cacheMutex(), portMAX_DELAY);
  if (expressionsSectionDirty_) {
    JsonWriter measure(nullptr, 0);
    writeExpressionsSection(measure, expressions);
    const size_t len = measure.length();
    if ((expressionsSectionJson_.capacity() >= len || heapCanSpare(len + 1)) &&
        resizeNoThrow(expressionsSectionJson_, len)) {
      JsonWriter w(expressionsSectionJson_.data(), len + 1);
      writeExpressionsSection(w, expressions);
      expressionsSectionDirty_ = false;
    } else {
      logSectionOom("expressions");
    }
  }
  xSemaphoreGive(cacheMutex());
}

void Config::expressionsSectionJsonCached(std::string& out) {
  expressionsSectionRebuildIfDirty();
  xSemaphoreTake(cacheMutex(), portMAX_DELAY);
  if (!((out.capacity() >= expressionsSectionJson_.size() ||
         heapCanSpare(expressionsSectionJson_.size())) &&
        assignNoThrow(out, expressionsSectionJson_.data(), expressionsSectionJson_.size()))) {
    out.clear();
    logSectionOom("expressions");
  }
  xSemaphoreGive(cacheMutex());
}

void Config::releaseSectionCaches() {
  xSemaphoreTake(cacheMutex(), portMAX_DELAY);
  for (auto* entry : {&lampSectionJson_, &baseSectionJson_, &shadeSectionJson_,
                      &expressionsSectionJson_, &homeSectionJson_}) {
    if (entry->capacity() > 15) std::string().swap(*entry);
  }
  lampSectionDirty_ = baseSectionDirty_ = shadeSectionDirty_ =
      expressionsSectionDirty_ = homeSectionDirty_ = true;
  xSemaphoreGive(cacheMutex());
}

#undef LAMP_DEFINE_SECTION_CACHED

void Config::invalidateLampSection() {
  lampSectionDirty_ = true;
}

void Config::invalidateBaseSection() {
  baseSectionDirty_ = true;
}

void Config::setDrawAnchors(uint16_t idleMa, uint16_t fullMa) {
  if (idleMa == drawIdleMa_ && fullMa == drawFullMa_) return;
  drawIdleMa_ = idleMa;
  drawFullMa_ = fullMa;
  invalidateBaseSection();
}

void Config::invalidateShadeSection() {
  shadeSectionDirty_ = true;
}

void Config::invalidateExpressionsSection() {
  expressionsSectionDirty_ = true;
}

void Config::invalidateHomeSection() {
  homeSectionDirty_ = true;
}

void Config::invalidateAllSections() {
  lampSectionDirty_ = true;
  baseSectionDirty_ = true;
  shadeSectionDirty_ = true;
  expressionsSectionDirty_ = true;
  homeSectionDirty_ = true;
}

void Config::applyDefaults(const Defaults& d) {
  // Called AFTER Config::Config(ConfigStore*) loads from NVS so that NVS
  // values are authoritative and only truly-factory-default fields are
  // overwritten. Subclass defaults() returns values that are used as
  // first-boot baselines.
  //
  // Name: resolve `named` (migrating pre-flag NVS from the stored name), then
  // substitute the variant default only for a lamp the user never named. A
  // named lamp keeps its stored name even when it equals a default word.
  lamp.named = resolveNamed(namedKeyPresent_, lamp.named, lamp.name, d.name);
  // A named lamp is already configured; never let the app re-adopt it (which
  // would reset its Web Config window) after an upgrade from firmware that
  // predates the setup flag.
  if (lamp.named) lamp.setup = true;
  if (!d.name.empty() && !lamp.named) {
    lamp.name = d.name;
  }
  // Colors: apply the variant's injected default when the vector still holds
  // only the single class-default entry. A user-configured lamp has a
  // different saved color and won't match, so its choice is preserved. A
  // multi-segment variant seeds its whole segment list instead of the scalar
  // single-color path.
  if (!d.baseSegments.empty()) {
    seedRoleSegments(base.segments, d.baseSegments, kBaseDefaultColor);
  } else if (!d.baseColor.empty() && base.broadcastColors().size() == 1 &&
             base.broadcastColors()[0] == kBaseDefaultColor) {
    base.broadcastColors() = parseColorList(d.baseColor);
  }
  if (!d.shadeSegments.empty()) {
    seedRoleSegments(shade.segments, d.shadeSegments, kShadeDefaultColor);
  } else if (!d.shadeColor.empty() && shade.broadcastColors().size() == 1 &&
             shade.broadcastColors()[0] == kShadeDefaultColor) {
    shade.broadcastColors() = parseColorList(d.shadeColor);
  }
  base.colorsEditable  = d.baseColorsEditable;
  shade.colorsEditable = d.shadeColorsEditable;
  if (d.setup) lamp.setup = true;

  // Per-segment pixel count. A loaded px of 0 means "unset" (a fresh lamp, or
  // a doc without the key). Fill those from the variant default; any real
  // stored value wins, so a configured lamp's px is sacred. The scalar
  // basePx/shadePx default seeds the role's first segment.
  base.segments.front().px = resolveConfiguredPx(base.segments.front().px, d.basePx);
  base.knockoutPixels.resize(base.sumPx(), 100);
  shade.segments.front().px = resolveConfiguredPx(shade.segments.front().px, d.shadePx);
}

}  // namespace lamp