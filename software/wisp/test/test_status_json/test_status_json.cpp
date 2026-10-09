#include <unity.h>
#include <ArduinoJson.h>
#include <cstdint>
#include "status/status_json.hpp"
#include "wire/lamp_protocol.hpp"

constexpr size_t CAP = 230;

// Guaranteed-core ceiling at worst-case field widths. If this grows past
// 179 the degrade-by-construction guarantee shrinks; re-derive the slack
// before bumping.
constexpr size_t CORE_PIN = 179;

void setUp() {}
void tearDown() {}

// Every optional field at its widest non-default value, Osc mode (oscRole +
// drift fields), both mismatch fields: the widest frame the builder can make.
static wisp::WispStatusFields worstCase() {
  return wisp::WispStatusFields{
      false, false, "abcdef12", 4294967295u, "osc", "secondary",
      255, 255, 255, 255, true, /*shuffleSeed=*/255,
      /*driftIntervalMs=*/3600000, /*driftFadePct=*/100,
      /*name=*/"12345678901234567890", /*hasPassword=*/true,
      /*ledType=*/"GRBW", /*pixelCount=*/65535,
      /*opSeq=*/4294967295u, /*brightness=*/99,
      /*wifiChannelMismatch=*/true, /*wifiApChannel=*/11 };
}

// Defaults everywhere, Manual mode.
static wisp::WispStatusFields plain() {
  return wisp::WispStatusFields{
      false, false, "", 1000u, "manual", "primary",
      0, 0, 0, 0, false, /*shuffleSeed=*/0,
      /*driftIntervalMs=*/120000, /*driftFadePct=*/50,
      /*name=*/"", /*hasPassword=*/false,
      /*ledType=*/"GRB", /*pixelCount=*/30 };
}

// Guaranteed core at pathological widths: 10-digit lastSeenMs, widest
// source, both bools false (wider than true), hasPassword present. All
// optional fields at their app defaults (omitted).
void test_core_worst_case_pinned_under_cap() {
  wisp::WispStatusFields f = plain();
  f.lastSeenMs = 4294967295u;
  f.hasPassword = true;
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_TRUE(n <= CORE_PIN);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_EQUAL_STRING("wispStatus", d["char"].as<const char*>());
  TEST_ASSERT_EQUAL_STRING("manual", d["source"].as<const char*>());
  TEST_ASSERT_FALSE(d["wifiConnected"].isNull());
  TEST_ASSERT_FALSE(d["sourceLive"].isNull());
  TEST_ASSERT_FALSE(d["lastSeenMs"].isNull());
  TEST_ASSERT_TRUE(d["hasPassword"].as<bool>());
  TEST_ASSERT_TRUE(d["auroraConnected"].isNull());
  TEST_ASSERT_TRUE(d["currentZone"].isNull());
  TEST_ASSERT_TRUE(d["zoneSource"].isNull());
}

// sourceLive rides the core both ways, so the app never reads absence.
void test_source_live_always_present() {
  wisp::WispStatusFields f = plain();
  f.source = "osc";
  f.sourceLive = true;
  char out[256];
  TEST_ASSERT_TRUE(wisp::buildWispStatusJson(f, out, sizeof(out), CAP) > 0);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_TRUE(d["sourceLive"].as<bool>());

  f.sourceLive = false;
  TEST_ASSERT_TRUE(wisp::buildWispStatusJson(f, out, sizeof(out), CAP) > 0);
  JsonDocument d2;
  TEST_ASSERT_FALSE(deserializeJson(d2, out));
  TEST_ASSERT_FALSE(d2["sourceLive"].isNull());
  TEST_ASSERT_FALSE(d2["sourceLive"].as<bool>());
}

// Every optional field set at pathological widths: the frame is always
// produced (never 0) and stays under cap; the core survives intact.
void test_fully_loaded_worst_case_never_fails() {
  wisp::WispStatusFields f = worstCase();
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_TRUE(n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_FALSE(d["source"].isNull());
  TEST_ASSERT_FALSE(d["sourceLive"].isNull());
  TEST_ASSERT_TRUE(d["hasPassword"].as<bool>());
}

// Priority: offColor and paletteIdPrefix survive budget pressure that
// drops the widest cosmetic (ledType).
void test_offcolor_and_prefix_outrank_cosmetics() {
  wisp::WispStatusFields f = worstCase();
  f.source = "off";
  f.wifiChannelMismatch = false;
  f.brightness = 100;
  f.opSeq = 0;
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_FALSE(d["offColor"].isNull());
  TEST_ASSERT_EQUAL_STRING("abcdef12", d["paletteIdPrefix"].as<const char*>());
  TEST_ASSERT_TRUE(d["oscRole"].isNull());
  TEST_ASSERT_TRUE(d["ledType"].isNull());
}

// Priority: in Osc mode oscRole takes offColor's slot and outranks the
// cosmetics the same way.
void test_osc_role_outranks_cosmetics() {
  wisp::WispStatusFields f = worstCase();
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_EQUAL_STRING("secondary", d["oscRole"].as<const char*>());
  TEST_ASSERT_EQUAL_STRING("abcdef12", d["paletteIdPrefix"].as<const char*>());
  TEST_ASSERT_TRUE(d["offColor"].isNull());
  TEST_ASSERT_TRUE(d["ledType"].isNull());
}

// oscRole rides only in Osc mode and only off its "primary" default.
void test_osc_role_omitted_at_default_and_outside_osc() {
  wisp::WispStatusFields f = plain();
  f.source = "osc";
  char out[256];
  TEST_ASSERT_TRUE(wisp::buildWispStatusJson(f, out, sizeof(out), CAP) > 0);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_TRUE(d["oscRole"].isNull());

  f.oscRole = "fill";
  TEST_ASSERT_TRUE(wisp::buildWispStatusJson(f, out, sizeof(out), CAP) > 0);
  JsonDocument d2;
  TEST_ASSERT_FALSE(deserializeJson(d2, out));
  TEST_ASSERT_EQUAL_STRING("fill", d2["oscRole"].as<const char*>());

  f.source = "manual";
  TEST_ASSERT_TRUE(wisp::buildWispStatusJson(f, out, sizeof(out), CAP) > 0);
  JsonDocument d3;
  TEST_ASSERT_FALSE(deserializeJson(d3, out));
  TEST_ASSERT_TRUE(d3["oscRole"].isNull());
}

// Omit-when-default: every field whose absence the app parser reconstructs
// is dropped from the wire when at its default.
void test_defaults_omitted() {
  wisp::WispStatusFields f = plain();
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_TRUE(d["hasPassword"].isNull());
  TEST_ASSERT_TRUE(d["shuffleSeed"].isNull());
  TEST_ASSERT_TRUE(d["driftIntervalMs"].isNull());
  TEST_ASSERT_TRUE(d["driftFadePct"].isNull());
  TEST_ASSERT_TRUE(d["name"].isNull());
  TEST_ASSERT_TRUE(d["ledType"].isNull());
  TEST_ASSERT_TRUE(d["px"].isNull());
  TEST_ASSERT_TRUE(d["paletteIdPrefix"].isNull());
  TEST_ASSERT_TRUE(d["oscRole"].isNull());
  TEST_ASSERT_TRUE(d["opSeq"].isNull());
}

// Non-default identity fields ride the wire when the frame has room.
void test_non_default_identity_fields_emitted() {
  wisp::WispStatusFields f = plain();
  f.shuffleSeed = 42;
  f.name = "lamp-room";
  f.hasPassword = true;
  f.ledType = "BGR";
  f.pixelCount = 42;
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_TRUE(d["hasPassword"].as<bool>());
  TEST_ASSERT_EQUAL_INT(42, d["shuffleSeed"].as<int>());
  TEST_ASSERT_EQUAL_STRING("lamp-room", d["name"].as<const char*>());
  TEST_ASSERT_EQUAL_STRING("BGR", d["ledType"].as<const char*>());
  TEST_ASSERT_EQUAL_INT(42, d["px"].as<int>());
}

// opSeq (sealed-op confirmation counter) rides the wire when non-default and
// the frame has room; omitted at 0.
void test_op_seq_emitted() {
  wisp::WispStatusFields f = plain();
  f.hasPassword = true;
  f.opSeq = 9;
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_EQUAL_UINT32(9, d["opSeq"].as<uint32_t>());
}

// Non-default palette and drift fields ride the wire when the frame has room.
void test_non_default_palette_and_drift_emitted() {
  wisp::WispStatusFields f = plain();
  f.paletteIdPrefix = "abcdef12";
  f.driftIntervalMs = 90000;
  f.driftFadePct = 40;
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_EQUAL_STRING("abcdef12", d["paletteIdPrefix"].as<const char*>());
  TEST_ASSERT_EQUAL_UINT32(90000, d["driftIntervalMs"].as<uint32_t>());
  TEST_ASSERT_EQUAL_INT(40, d["driftFadePct"].as<int>());
}

// Off mode emits offColor and suppresses the drift fields even when they
// are non-default.
void test_off_mode_emits_offcolor_not_drift() {
  wisp::WispStatusFields f = plain();
  f.source = "off";
  f.offR = 200; f.offG = 100; f.offB = 50; f.offW = 25;
  f.hasOffColor = true;
  f.driftIntervalMs = 90000;
  f.driftFadePct = 40;
  char out[256];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_EQUAL_STRING("c8643219", d["offColor"].as<const char*>());
  TEST_ASSERT_TRUE(d["driftIntervalMs"].isNull());
  TEST_ASSERT_TRUE(d["driftFadePct"].isNull());
}

// brightness omitted at the 100 default, emitted when the space is dimmed.
void test_brightness_omitted_at_default_emitted_when_dimmed() {
  wisp::WispStatusFields fDefault = plain();
  char out[256];
  size_t n = wisp::buildWispStatusJson(fDefault, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, out));
  TEST_ASSERT_TRUE(d["brightness"].isNull());

  wisp::WispStatusFields fDimmed = fDefault;
  fDimmed.brightness = 40;
  n = wisp::buildWispStatusJson(fDimmed, out, sizeof(out), CAP);
  TEST_ASSERT_TRUE(n > 0 && n <= CAP);
  JsonDocument d2;
  TEST_ASSERT_FALSE(deserializeJson(d2, out));
  TEST_ASSERT_EQUAL_INT(40, d2["brightness"].as<int>());
}

// Absolute worst case, untruncated: every field widest, Osc mode, both
// mismatch fields. It fits CONTROL_MAX_PAYLOAD with room to spare, so at the
// production cap nothing sheds.
void test_worst_case_untruncated_length() {
  wisp::WispStatusFields f = worstCase();
  char out[1024];
  size_t n = wisp::buildWispStatusJson(f, out, sizeof(out), /*cap=*/4096);
  TEST_ASSERT_EQUAL_UINT32(373, n);
  TEST_ASSERT_TRUE(n <= lamp_protocol::CONTROL_MAX_PAYLOAD);
}

// The real emitter path: worst-case fields built at the production cap
// (CONTROL_MAX_PAYLOAD), fed straight into buildControlOp. Every field
// survives and the frame builds.
void test_worst_case_fits_control_op_at_production_cap() {
  wisp::WispStatusFields f = worstCase();
  char json[1024];
  size_t jsonLen = wisp::buildWispStatusJson(
      f, json, sizeof(json), lamp_protocol::CONTROL_MAX_PAYLOAD);
  TEST_ASSERT_EQUAL_UINT32(373, jsonLen);
  JsonDocument d;
  TEST_ASSERT_FALSE(deserializeJson(d, json));
  TEST_ASSERT_EQUAL_STRING("secondary", d["oscRole"].as<const char*>());
  TEST_ASSERT_EQUAL_INT(65535, d["px"].as<int>());
  TEST_ASSERT_TRUE(d["wifiChannelMismatch"].as<bool>());
  TEST_ASSERT_EQUAL_INT(11, d["wifiApChannel"].as<int>());
  TEST_ASSERT_EQUAL_INT(99, d["brightness"].as<int>());

  static const uint8_t kTarget[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  static const uint8_t kSrc[6]    = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
  uint8_t frame[lamp_protocol::CONTROL_MAX_SIZE];
  size_t frameLen = lamp_protocol::buildControlOp(
      frame, sizeof(frame), /*seq=*/1, kTarget, kSrc,
      reinterpret_cast<const uint8_t*>(json), jsonLen);
  TEST_ASSERT_TRUE(frameLen > 0);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_core_worst_case_pinned_under_cap);
  RUN_TEST(test_source_live_always_present);
  RUN_TEST(test_worst_case_untruncated_length);
  RUN_TEST(test_worst_case_fits_control_op_at_production_cap);
  RUN_TEST(test_brightness_omitted_at_default_emitted_when_dimmed);
  RUN_TEST(test_fully_loaded_worst_case_never_fails);
  RUN_TEST(test_offcolor_and_prefix_outrank_cosmetics);
  RUN_TEST(test_osc_role_outranks_cosmetics);
  RUN_TEST(test_osc_role_omitted_at_default_and_outside_osc);
  RUN_TEST(test_defaults_omitted);
  RUN_TEST(test_non_default_identity_fields_emitted);
  RUN_TEST(test_op_seq_emitted);
  RUN_TEST(test_non_default_palette_and_drift_emitted);
  RUN_TEST(test_off_mode_emits_offcolor_not_drift);
  return UNITY_END();
}
