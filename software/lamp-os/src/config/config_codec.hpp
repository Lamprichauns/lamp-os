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
// `lamp` null leaves out the lamp section; `whole` non-null adds
// base.knockout and the expressions array, the shape of a whole save.
// writeLampOnlyJson writes {"lamp":{...}}, the split save's cfg string.
void writeLampOnlyJson(JsonWriter& w, const LampSettings& lamp);
void writeStoredJson(JsonWriter& w, const LampSettings* lamp, const BaseSettings& base,
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

// Invalid and NoHeap come from saveRawJson only, and leave the store
// untouched, as TooLarge does.
enum class SaveResult : uint8_t { Ok, WriteFailed, TooLarge, Invalid, NoHeap };

// The save buffer's length: it holds a whole config up to kMaxWholeCfgLen
// and a split save's rest at the caps with every escapable string fully
// escaped (1746 bytes, measured by test_config_records).
constexpr size_t kMaxStoredCfgJsonLen = 1792;
// The largest whole config saved as the one "cfg" string. A string must fit
// in a single NVS page; at this size NVS can always place it with every
// other key at its cap (docs/dev/embedded-heap.md, pinned by
// test_config_records).
constexpr size_t kMaxWholeCfgLen = 1759;
static_assert(kMaxWholeCfgLen <= kMaxStoredCfgJsonLen, "the save buffer holds a whole config");

// Saves the model. A whole config up to kMaxWholeCfgLen is one "cfg"
// string carrying the expressions and knockout too, the form any firmware
// reads; when NVS cannot place it beside the split keys it is staged as the
// "cfgb" blob first, and the split keys are erased once it is authoritative.
// A larger config splits: each expression as a record ("x0", "x1", ...), the
// knockout levels in "ko", the count in "xn", record slots past the count
// erased, the rest but the lamp section as the "cfgb" blob, and last the lamp
// section as the "cfg" string, which an older firmware reads too. Every value
// but the cfg string is a blob NVS may spread across pages. The JSON is built
// before any write, so TooLarge leaves the store untouched; an unchanged key
// is not rewritten (NVS skips identical writes), and a save needs no heap.
// Stops at the first failed write; the caller keeps its dirty state and
// retries.
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

// Overlays what a split save keeps outside the stored JSON. The JSON wins
// wherever it carries the data itself: knockout levels from "ko" only when it
// has no base.knockout, expressions from their records only when it has no
// expressions array (an older firmware always writes both, so a config it
// saved after a downgrade is what loads). A record that fails to read drops
// only itself (droppedMask); a repeated (type, target) keeps its first record.
// Call after fromJson, which sizes the knockout.
LoadedRecords loadRecords(ConfigStore& store, BaseSettings& base,
                          ExpressionSettings& expressions, bool cfgHasKnockout,
                          bool cfgHasExpressions);

// Loads a stored config from the parsed "cfg" string (`root`): whole when it
// carries an expressions array (the authority over every other key), else
// from "cfgb", whole when it carries one and otherwise the rest beside the
// records, the knockout blob and the cfg string's lamp section. `withinCaps`
// reports fromJson's result.
LoadedRecords loadStoredConfig(ConfigStore& store, JsonObject root, LampSettings& lamp,
                               BaseSettings& base, ShadeSettings& shade,
                               ExpressionSettings& expressions,
                               HomeModeSettings& homeMode, bool& withinCaps);

// Heap a raw save may take beyond its document: the parse holds about twice
// the document, the strings copied out of it once more, and the model at the
// caps stays under kRawSaveModelBytes.
constexpr size_t kRawSaveHeapPerByte = 3;
constexpr size_t kRawSaveModelBytes = 8192;

// Saves a whole config document (the webapp's whole-document PUT) through
// fromJson and saveConfig, for the next boot to load. Invalid when it does
// not parse or carries no `lamp` object, so a truncated body cannot replace a
// good config; NoHeap when the heap cannot spare the parse and the model
// (heapCanSpare), so no allocation on the way can throw.
SaveResult saveRawJson(ConfigStore& store, const char* json, char* cfgBuffer,
                       size_t cfgBufferLen);

}  // namespace config_codec
}  // namespace lamp
