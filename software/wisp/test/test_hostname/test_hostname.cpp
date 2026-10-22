#include <unity.h>

#include "net/hostname.hpp"

void setUp() {}
void tearDown() {}

static void check(const char* want, const char* name, size_t cap = 32) {
  char out[64];
  wisp::toHostname(name, "wisp", out, cap);
  TEST_ASSERT_EQUAL_STRING(want, out);
}

void test_lowercases_and_keeps_hostname_chars() {
  check("wisp-stage-left", "wisp-Stage-Left");
}

void test_invalid_runs_become_one_hyphen() {
  check("wisp-living-room", "wisp-living  room!");
  check("wisp-caf", "wisp-café");
}

void test_no_leading_or_trailing_hyphen() {
  check("a-b", "--a__b--");
}

void test_empty_falls_back() {
  check("wisp", "");
  check("wisp", "!!!");
  check("wisp", nullptr);
}

void test_truncates_to_cap_without_trailing_hyphen() {
  check("wisp", "wisp-abcdef", /*cap=*/6);
  check("wisp-a", "wisp-abcdef", /*cap=*/7);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lowercases_and_keeps_hostname_chars);
  RUN_TEST(test_invalid_runs_become_one_hyphen);
  RUN_TEST(test_no_leading_or_trailing_hyphen);
  RUN_TEST(test_empty_falls_back);
  RUN_TEST(test_truncates_to_cap_without_trailing_hyphen);
  return UNITY_END();
}
