// Native-host test for apply::colorHarmonyLocal, the settings_blob path for a
// role's autoColors and colorScheme. Absent or invalid keys keep stored values.

#include <unity.h>

#include <cstdint>

#include <ArduinoJson.h>

#include "components/apply/apply_color_harmony.hpp"

namespace lamp {
struct SurfaceMirror {
  bool autoColors = false;
  uint8_t colorScheme = 0;
};
}  // namespace lamp

void setUp(void) {}
void tearDown(void) {}

static JsonDocument blob(const char* json) {
  JsonDocument doc;
  deserializeJson(doc, json);
  return doc;
}

static lamp::SurfaceMirror stored() {
  lamp::SurfaceMirror s;
  s.autoColors = true;
  s.colorScheme = 7;
  return s;
}

static void expectUnchanged(const char* json) {
  lamp::SurfaceMirror s = stored();
  JsonDocument doc = blob(json);
  lamp::apply::colorHarmonyLocal(doc.as<JsonObjectConst>(), s);
  TEST_ASSERT_TRUE(s.autoColors);
  TEST_ASSERT_EQUAL_UINT8(7, s.colorScheme);
}

void test_absent_keys_leave_stored_values() {
  expectUnchanged("{\"colors\":[]}");
}

void test_valid_values_applied() {
  lamp::SurfaceMirror s;
  JsonDocument doc = blob("{\"autoColors\":true,\"colorScheme\":200}");
  lamp::apply::colorHarmonyLocal(doc.as<JsonObjectConst>(), s);
  TEST_ASSERT_TRUE(s.autoColors);
  TEST_ASSERT_EQUAL_UINT8(200, s.colorScheme);
}

void test_false_and_zero_override_stored_values() {
  lamp::SurfaceMirror s = stored();
  JsonDocument doc = blob("{\"autoColors\":false,\"colorScheme\":0}");
  lamp::apply::colorHarmonyLocal(doc.as<JsonObjectConst>(), s);
  TEST_ASSERT_FALSE(s.autoColors);
  TEST_ASSERT_EQUAL_UINT8(0, s.colorScheme);
}

void test_color_scheme_255_applied() {
  lamp::SurfaceMirror s = stored();
  JsonDocument doc = blob("{\"colorScheme\":255}");
  lamp::apply::colorHarmonyLocal(doc.as<JsonObjectConst>(), s);
  TEST_ASSERT_TRUE(s.autoColors);
  TEST_ASSERT_EQUAL_UINT8(255, s.colorScheme);
}

void test_auto_colors_wrong_type_ignored() {
  expectUnchanged("{\"autoColors\":1}");
  expectUnchanged("{\"autoColors\":\"true\"}");
  expectUnchanged("{\"autoColors\":null}");
}

void test_color_scheme_invalid_ignored() {
  expectUnchanged("{\"colorScheme\":256}");
  expectUnchanged("{\"colorScheme\":-1}");
  expectUnchanged("{\"colorScheme\":3.5}");
  expectUnchanged("{\"colorScheme\":\"3\"}");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_absent_keys_leave_stored_values);
  RUN_TEST(test_valid_values_applied);
  RUN_TEST(test_false_and_zero_override_stored_values);
  RUN_TEST(test_color_scheme_255_applied);
  RUN_TEST(test_auto_colors_wrong_type_ignored);
  RUN_TEST(test_color_scheme_invalid_ignored);
  return UNITY_END();
}
