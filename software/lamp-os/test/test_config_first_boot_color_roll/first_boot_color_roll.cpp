// Native-host test for the first-boot color roll's "is this lamp still
// factory?" predicate. A variant that omits baseColor/shadeColor keeps the
// class default, so comparing a role against the variant's own (empty) hex
// string reports every factory lamp as user-configured and the roll never
// fires. StandardLamp omits baseColor, which stranded every unit on the same
// #300783 base + #5A1700 shade.

#include <unity.h>

#include <string>
#include <vector>

#include "config/config_types.hpp"

// The native env doesn't link src/; pull in the hex parser the predicate uses.
#include "../../src/util/color.cpp"

void setUp(void) {}
void tearDown(void) {}

// StandardLamp::defaults(): baseColor omitted, shadeColor injected.
static const std::string kVariantBase = "";
static const std::string kVariantShade = "#5A170000";
static const lamp::Color kVariantShadeColor(0x5A, 0x17, 0x00, 0x00);

void test_omitted_variant_color_still_reads_factory() {
  TEST_ASSERT_TRUE(lamp::colorsAreFactoryDefault(
      {lamp::kBaseDefaultColor}, {kVariantShadeColor},
      kVariantBase, kVariantShade));
}

void test_user_base_color_is_not_factory() {
  const lamp::Color chosen(0x11, 0x22, 0x33, 0x00);
  TEST_ASSERT_FALSE(lamp::colorsAreFactoryDefault(
      {chosen}, {kVariantShadeColor}, kVariantBase, kVariantShade));
}

void test_user_shade_color_is_not_factory() {
  const lamp::Color chosen(0x11, 0x22, 0x33, 0x00);
  TEST_ASSERT_FALSE(lamp::colorsAreFactoryDefault(
      {lamp::kBaseDefaultColor}, {chosen}, kVariantBase, kVariantShade));
}

// A variant that injects both colors compares against those, not the class
// defaults.
void test_variant_supplied_colors_match() {
  const lamp::Color vBase(0x01, 0x02, 0x03, 0x04);
  const lamp::Color vShade(0x05, 0x06, 0x07, 0x08);
  TEST_ASSERT_TRUE(lamp::colorsAreFactoryDefault(
      {vBase}, {vShade}, "#01020304", "#05060708"));
  TEST_ASSERT_FALSE(lamp::colorsAreFactoryDefault(
      {lamp::kBaseDefaultColor}, {vShade}, "#01020304", "#05060708"));
}

// A multi-stop gradient is a user edit whatever the stops are.
void test_multi_stop_is_not_factory() {
  TEST_ASSERT_FALSE(lamp::colorsAreFactoryDefault(
      {lamp::kBaseDefaultColor, lamp::kBaseDefaultColor}, {kVariantShadeColor},
      kVariantBase, kVariantShade));
  TEST_ASSERT_FALSE(lamp::colorsAreFactoryDefault(
      {}, {kVariantShadeColor}, kVariantBase, kVariantShade));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_omitted_variant_color_still_reads_factory);
  RUN_TEST(test_user_base_color_is_not_factory);
  RUN_TEST(test_user_shade_color_is_not_factory);
  RUN_TEST(test_variant_supplied_colors_match);
  RUN_TEST(test_multi_stop_is_not_factory);
  return UNITY_END();
}
