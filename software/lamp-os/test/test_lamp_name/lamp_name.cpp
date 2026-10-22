// Native-host tests for normalizeLampName: the canonical stored form every
// name write funnels through (BLE settings_blob, serial cfg.set, NVS load).

#include <unity.h>

#include "../../src/util/str.hpp"

using lamp::normalizeLampName;

void setUp(void) {}
void tearDown(void) {}

void test_lowercases_like_asciilower_did() {
  TEST_ASSERT_EQUAL_STRING("gramp", normalizeLampName("GRAMP").c_str());
  TEST_ASSERT_EQUAL_STRING("gramp", normalizeLampName("Gramp").c_str());
}

// The trailing space that made a lamp advertise "gramp -lamp".
void test_drops_trailing_and_leading_space() {
  TEST_ASSERT_EQUAL_STRING("gramp", normalizeLampName("gramp ").c_str());
  TEST_ASSERT_EQUAL_STRING("gramp", normalizeLampName(" gramp").c_str());
  TEST_ASSERT_EQUAL_STRING("gramp", normalizeLampName("  gramp  ").c_str());
}

void test_drops_interior_whitespace() {
  TEST_ASSERT_EQUAL_STRING("bigjim", normalizeLampName("big jim").c_str());
  TEST_ASSERT_EQUAL_STRING("bigjim", normalizeLampName("big\tjim").c_str());
  TEST_ASSERT_EQUAL_STRING("bigjim", normalizeLampName("big\njim").c_str());
}

void test_leaves_a_clean_name_alone() {
  TEST_ASSERT_EQUAL_STRING("gramp", normalizeLampName("gramp").c_str());
  TEST_ASSERT_EQUAL_STRING("lamp2", normalizeLampName("lamp2").c_str());
}

void test_all_whitespace_collapses_to_empty() {
  TEST_ASSERT_EQUAL_STRING("", normalizeLampName("   ").c_str());
  TEST_ASSERT_EQUAL_STRING("", normalizeLampName("").c_str());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lowercases_like_asciilower_did);
  RUN_TEST(test_drops_trailing_and_leading_space);
  RUN_TEST(test_drops_interior_whitespace);
  RUN_TEST(test_leaves_a_clean_name_alone);
  RUN_TEST(test_all_whitespace_collapses_to_empty);
  return UNITY_END();
}
