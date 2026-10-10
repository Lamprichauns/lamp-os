// Pins that the fixed-capacity ExpressionParams serializes byte-identically to
// the std::map<std::string, uint32_t> it replaced, on every wire that carries
// expression parameters: the mesh invocation JSON (MSG_COMMAND / MSG_EVENT),
// and the expression object the BLE expressions section and the webapp
// document share. The map-based serializers are reproduced here as they
// shipped, so any drift in key order or value encoding fails.

#include <unity.h>

#include <ArduinoJson.h>

#include <map>
#include <string>
#include <vector>

#include "config/config_codec.hpp"
#include "expressions/expression_invocation.hpp"

#include "../../src/util/color.cpp"
#include "../../src/util/heap_guard.cpp"
#include "../../src/config/config_codec.cpp"
#include "../../src/expressions/expression_invocation.cpp"

using namespace lamp;

void setUp(void) {}
void tearDown(void) {}

namespace {

using LegacyParams = std::map<std::string, uint32_t>;

std::string legacyInvocationJson(const ExpressionInvocation& inv,
                                 const LegacyParams& params) {
  JsonDocument doc;
  doc["type"] = inv.type;
  doc["target"] = inv.target;
  doc["delayMs"] = inv.delayMs;
  if (!inv.colors.empty()) doc["colors"] = colorsToPackedHex(inv.colors);
  JsonObject paramsObj = doc["parameters"].to<JsonObject>();
  for (const auto& kv : params) paramsObj[kv.first] = kv.second;
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string legacyExpressionJson(const ExpressionConfig& expr,
                                 const LegacyParams& params) {
  JsonDocument doc;
  JsonObject exprNode = doc.to<JsonObject>();
  exprNode["type"] = expr.type;
  exprNode["enabled"] = expr.enabled;
  exprNode["intervalMin"] = expr.intervalMin;
  exprNode["intervalMax"] = expr.intervalMax;
  exprNode["target"] = expr.target;
  for (const auto& param : params) exprNode[param.first] = param.second;
  JsonArray colorsNode = exprNode["colors"].to<JsonArray>();
  for (const auto& color : expr.colors) colorsNode.add(colorToHexString(color));
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string expressionJson(const ExpressionConfig& expr) {
  JsonDocument doc;
  config_codec::expressionToJson(doc.to<JsonObject>(), expr);
  std::string out;
  serializeJson(doc, out);
  return out;
}

// Parameter sets inserted out of key order, from one key to every key.
std::vector<std::vector<std::pair<const char*, uint32_t>>> paramSets() {
  std::vector<std::vector<std::pair<const char*, uint32_t>>> sets = {
      {},
      {{"size", 4}},
      {{"spotSpeed", 3}, {"count", 5}, {"cascadeStaggerMs", 200}, {"cascadeEnabled", 1}},
      {{"shiftDurationMin", 300}, {"fillMode", 2}, {"fadeDuration", 60},
       {"shiftDurationMax", 900}, {"easing", 17}, {"opacity", 100}},
      {{"posMax", 4294967295u}, {"posMin", 0}, {"fullStrip", 0}, {"loop", 1}},
  };
  std::vector<std::pair<const char*, uint32_t>> all;
  for (size_t i = kExpressionParamKeyCount; i-- > 0;) {
    all.push_back({kExpressionParamKeys[i], static_cast<uint32_t>(i * 1000 + 7)});
  }
  sets.push_back(all);
  return sets;
}

}  // namespace

void test_invocation_json_matches_map_serializer() {
  for (const auto& set : paramSets()) {
    ExpressionInvocation inv;
    inv.type = "glitchy";
    inv.target = 3;
    inv.delayMs = 250;
    inv.colors = {Color(1, 2, 3, 4), Color(255, 0, 128, 9)};
    LegacyParams legacy;
    for (const auto& kv : set) {
      inv.parameters.set(kv.first, kv.second);
      legacy[kv.first] = kv.second;
    }
    std::string out;
    serializeInvocation(inv, out);
    TEST_ASSERT_EQUAL_STRING(legacyInvocationJson(inv, legacy).c_str(), out.c_str());
  }
}

void test_expression_json_matches_map_serializer() {
  for (const auto& set : paramSets()) {
    ExpressionConfig expr;
    expr.type = "shifty";
    expr.enabled = true;
    expr.intervalMin = 300;
    expr.intervalMax = 18000;
    expr.target = 2;
    expr.colors = {Color(16, 32, 48, 0)};
    LegacyParams legacy;
    for (const auto& kv : set) {
      expr.setParameter(kv.first, kv.second);
      legacy[kv.first] = kv.second;
    }
    TEST_ASSERT_EQUAL_STRING(legacyExpressionJson(expr, legacy).c_str(),
                             expressionJson(expr).c_str());
  }
}

// One literal so the pin holds even if both serializers drift together.
void test_invocation_json_golden() {
  ExpressionInvocation inv;
  inv.type = "pulse";
  inv.target = 1;
  inv.colors = {Color(255, 0, 0, 0)};
  inv.parameters = {{"size", 4}, {"easing", 4}, {"pulseSpeed", 7}};
  std::string out;
  serializeInvocation(inv, out);
  TEST_ASSERT_EQUAL_STRING(
      "{\"type\":\"pulse\",\"target\":1,\"delayMs\":0,\"colors\":\"ff000000\","
      "\"parameters\":{\"easing\":4,\"pulseSpeed\":7,\"size\":4}}",
      out.c_str());
}

// A payload a map-based peer sends parses and re-serializes unchanged, and a
// key no expression reads is dropped at parse.
void test_parse_round_trips_and_drops_unknown_keys() {
  const char* wire =
      "{\"type\":\"glitchy\",\"target\":3,\"delayMs\":0,\"colors\":\"ff000000\","
      "\"parameters\":{\"durationMax\":900,\"durationMin\":250,\"scatter\":3}}";
  JsonDocument doc;
  deserializeJson(doc, wire);
  ExpressionInvocation inv;
  TEST_ASSERT_TRUE(parseInvocation(doc.as<JsonObjectConst>(), inv));
  std::string out;
  serializeInvocation(inv, out);
  TEST_ASSERT_EQUAL_STRING(wire, out.c_str());

  JsonDocument withUnknown;
  deserializeJson(withUnknown,
                  "{\"type\":\"glitchy\",\"parameters\":{\"bogus\":1,\"scatter\":3}}");
  ExpressionInvocation parsed;
  TEST_ASSERT_TRUE(parseInvocation(withUnknown.as<JsonObjectConst>(), parsed));
  TEST_ASSERT_EQUAL_UINT(1, parsed.parameters.size());
}

// Key-index order is the byte-wise key order a std::map iterated in, which is
// what makes the wire identical. An appended key that breaks it changes the
// JSON key order on the wire; this fails first so the change is a decision.
void test_key_table_is_in_map_order() {
  for (size_t i = 1; i < kExpressionParamKeyCount; ++i) {
    TEST_ASSERT_TRUE_MESSAGE(
        std::string(kExpressionParamKeys[i - 1]) < std::string(kExpressionParamKeys[i]),
        kExpressionParamKeys[i]);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_invocation_json_matches_map_serializer);
  RUN_TEST(test_expression_json_matches_map_serializer);
  RUN_TEST(test_invocation_json_golden);
  RUN_TEST(test_parse_round_trips_and_drops_unknown_keys);
  RUN_TEST(test_key_table_is_in_map_order);
  return UNITY_END();
}
