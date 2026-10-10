#pragma once

#include <ArduinoJson.h>

#include <cstddef>
#include <cstdint>

#include "config_store.hpp"
#include "util/json_writer.hpp"
#include "config_types.hpp"

// JSON codec for the persisted "cfg" blob. Pure functions over the config
// model structs (no NVS, no Serial), so the parse/serialize logic runs in
// the native suite (round-trip tested in test/test_config_codec). Config owns
// the store, the section cache, and logging; the byte shape lives here.
namespace lamp {
namespace config_codec {

// The stored parameters for (type, target), or null when the list has no
// such entry.
const ExpressionParams* findExpressionParams(const ExpressionSettings& settings,
                                             const std::string& type, uint8_t target);

// Mirrors an upsert into the stored list: replaces the (type, target) entry,
// or appends while under kMaxExpressions, growing the vector four slots at a
// time. False when the list is full and the entry was not stored.
bool mirrorUpsert(ExpressionSettings& settings, const ExpressionConfig& cfg);

// `s` when it names a byte order the strip driver knows ("GRBW", "GRB",
// "BGR"), else "" (the variant's default). Bounds the stored field.
std::string storableByteOrder(const char* s);

// Parse one expression entry (a stored "expressions" element or an upsert
// op's "entry"), dropping colours and parameters past the caps. The target is
// copied as given. Returns false when a cap dropped something.
bool parseExpression(JsonObject node, ExpressionConfig& out, bool enabledDefault);

// Parse the "cfg" blob into the model. Applies the field defaults, clamps,
// byte-order derivation, and legacy migrations the loader relies on. Leaves
// each struct at its class default for any absent key. Returns false when an
// expression cap truncated the stored list; the caller logs it, and the next
// save writes the truncated form.
bool fromJson(JsonObject root, LampSettings& lamp, BaseSettings& base,
              ShadeSettings& shade, ExpressionSettings& expressions,
              HomeModeSettings& homeMode);

// Serialize the model into root (the canonical "cfg" blob shape, also what
// the constructor re-parses on the next boot).
void toJson(JsonObject root, const LampSettings& lamp, const BaseSettings& base,
            const ShadeSettings& shade, const ExpressionSettings& expressions,
            const HomeModeSettings& homeMode);

// The same shapes written with no heap, byte-identical to serializeJson of
// expressionToJson / toStoredJson (pinned by test_json_writer).
void writeExpressionJson(JsonWriter& w, const ExpressionConfig& expr);
// `whole` non-null adds base.knockout and the expressions array, the shape
// of a whole save.
void writeStoredJson(JsonWriter& w, const LampSettings& lamp, const BaseSettings& base,
                     const ShadeSettings& shade, const HomeModeSettings& homeMode,
                     const ExpressionSettings* whole);

// One expression's JSON object, the shape the BLE expressions section and the
// webapp document both carry: type, enabled, the interval pair, target, each
// parameter as its own key in key order, then colours.
void expressionToJson(JsonObject node, const ExpressionConfig& expr);

// Serialize everything but the expressions and the knockout levels (the "cfg"
// key's stored shape).
void toStoredJson(JsonObject root, const LampSettings& lamp,
                  const BaseSettings& base, const ShadeSettings& shade,
                  const HomeModeSettings& homeMode);

// One expression as a compact record: a format byte, flags, target, the type
// id, the interval pair, RGBW colours, then (param key index, value) pairs.
// Longest at the caps is kMaxExpressionRecordLen. Parameters whose key has no
// index are skipped.
constexpr size_t kMaxExpressionRecordLen =
    4 + kMaxExpressionTypeLen + 8 + 1 + 4 * kMaxExpressionColors + 1 +
    5 * kMaxExpressionParams;
size_t encodeExpression(const ExpressionConfig& expr, uint8_t* out);
bool decodeExpression(const uint8_t* data, size_t len, ExpressionConfig& out);

enum class SaveResult : uint8_t { Ok, WriteFailed, TooLarge };

// Ceiling of the "cfg" JSON at the caps with every escapable string fully
// escaped (measured by test_config_records); saveConfig writes it into a
// buffer this long.
constexpr size_t kMaxStoredCfgJsonLen = 2176;

// Saves the model. When the whole config fits `cfgBuffer` (2 KB) it is one
// "cfg" JSON carrying the expressions and knockout too, the form any firmware
// reads, and the split keys from an earlier save are erased after it lands.
// Otherwise it splits: each expression as a record ("x0", "x1", ...), the
// knockout levels in "ko", the count in "xn" (written after the records),
// records past the count erased, then the rest as the "cfg" JSON. The cfg
// JSON is built before any write, so TooLarge leaves the store untouched. No
// single value outgrows an NVS page, an unchanged key is not rewritten (NVS
// skips identical writes), and a save needs no heap. Stops at the first
// failed write; the caller keeps its dirty state and retries.
SaveResult saveConfig(ConfigStore& store, const LampSettings& lamp,
                      const BaseSettings& base, const ShadeSettings& shade,
                      const ExpressionSettings& expressions,
                      const HomeModeSettings& homeMode, char* cfgBuffer,
                      size_t cfgBufferLen);

struct LoadedRecords {
  bool present = false;      // the expressions came from records
  uint32_t droppedMask = 0;  // bit i: record "x<i>" missing or unreadable
  uint8_t duplicates = 0;    // records skipped as a repeat (type, target)
};

// Overlays what a split save keeps outside the "cfg" JSON. The cfg JSON wins
// wherever it carries the data itself: knockout levels from "ko" only when it
// has no base.knockout, expressions from their records only when it has no
// expressions array (an older firmware always writes both, so a config it
// saved after a downgrade is what loads). A record that fails to read drops
// only itself (droppedMask); a repeated (type, target) keeps its first record.
// Call after fromJson, which sizes the knockout.
LoadedRecords loadRecords(ConfigStore& store, BaseSettings& base,
                          ExpressionSettings& expressions, bool cfgHasKnockout,
                          bool cfgHasExpressions);

// Loads a stored config: the parsed "cfg" JSON (`root`) through fromJson,
// then loadRecords with the cfg JSON's own expressions and knockout as the
// authority. `withinCaps` reports fromJson's result.
LoadedRecords loadStoredConfig(ConfigStore& store, JsonObject root, LampSettings& lamp,
                               BaseSettings& base, ShadeSettings& shade,
                               ExpressionSettings& expressions,
                               HomeModeSettings& homeMode, bool& withinCaps);

// True when `json` is a whole config document safe to persist: it parses and
// carries a `lamp` object. persistRawJson gates on this so a truncated or
// unparseable blob (Arduino String silently truncates on a fragmented-heap
// OOM) can't overwrite a good stored config. A parse OOM returns non-Ok, so
// it also rejects rather than persisting a partial write.
bool configBlobPersistable(const char* json);

}  // namespace config_codec
}  // namespace lamp
