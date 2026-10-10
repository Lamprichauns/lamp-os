#include <unity.h>

#include <cmath>
#include <cstdint>

#include "util/dither.hpp"

using lamp::ditherStep;
using lamp::gammaLevel88;
using lamp::kGamma88;
using lamp::roundedLevel;

void setUp() {}
void tearDown() {}

void test_table_is_gamma_2_6_in_8_8() {
  for (int i = 0; i < 256; i++) {
    const double expected = std::pow(i / 255.0, 2.6) * 65280.0;
    TEST_ASSERT_TRUE(std::fabs(kGamma88[i] - expected) <= 0.5);
  }
  TEST_ASSERT_EQUAL_UINT16(65280, kGamma88[255]);
}

void test_level_scales_and_full_level_is_verbatim() {
  for (int v = 0; v < 256; v++) {
    TEST_ASSERT_EQUAL_UINT16(kGamma88[v], gammaLevel88(v, 255));
    TEST_ASSERT_EQUAL_UINT16(0, gammaLevel88(v, 0));
  }
  TEST_ASSERT_EQUAL_UINT16(0, gammaLevel88(0, 200));
  TEST_ASSERT_EQUAL_UINT16(32768, gammaLevel88(255, 128));
}

// Every 8.8 target, from every starting carry, averages to the target within
// one 8-bit step over any window and lands exact over 256 frames.
void test_average_tracks_target() {
  const uint16_t targets[] = {1, 37, 128, 255, 256, 300, 1000, 12345, 40000, 65279, 65280};
  for (uint16_t t : targets) {
    for (int seed = 0; seed < 256; seed += 51) {
      uint8_t carry = static_cast<uint8_t>(seed);
      uint32_t sum = 0;
      for (int f = 1; f <= 256; f++) {
        sum += ditherStep(t, carry);
        const double avg = static_cast<double>(sum) / f;
        TEST_ASSERT_TRUE(std::fabs(avg - t / 256.0) * f <= 1.0);
      }
      TEST_ASSERT_EQUAL_UINT32(t, sum);
    }
  }
}

void test_zero_stays_off_whatever_the_carry() {
  for (int seed = 0; seed < 256; seed++) {
    uint8_t carry = static_cast<uint8_t>(seed);
    for (int f = 0; f < 8; f++) TEST_ASSERT_EQUAL_UINT8(0, ditherStep(0, carry));
  }
}

void test_whole_levels_are_exact() {
  for (int seed = 0; seed < 256; seed++) {
    uint8_t carry = static_cast<uint8_t>(seed);
    for (int f = 0; f < 8; f++) {
      TEST_ASSERT_EQUAL_UINT8(255, ditherStep(gammaLevel88(255, 255), carry));
      TEST_ASSERT_EQUAL_UINT8(42, ditherStep(42 << 8, carry));
    }
  }
}

void test_rounded_level_is_nearest_and_exact_at_the_ends() {
  TEST_ASSERT_EQUAL_UINT8(0, roundedLevel(0));
  TEST_ASSERT_EQUAL_UINT8(0, roundedLevel(127));
  TEST_ASSERT_EQUAL_UINT8(1, roundedLevel(128));
  TEST_ASSERT_EQUAL_UINT8(3, roundedLevel(895));
  TEST_ASSERT_EQUAL_UINT8(4, roundedLevel(896));
  TEST_ASSERT_EQUAL_UINT8(255, roundedLevel(65280));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_table_is_gamma_2_6_in_8_8);
  RUN_TEST(test_level_scales_and_full_level_is_verbatim);
  RUN_TEST(test_average_tracks_target);
  RUN_TEST(test_zero_stays_off_whatever_the_carry);
  RUN_TEST(test_whole_levels_are_exact);
  RUN_TEST(test_rounded_level_is_nearest_and_exact_at_the_ends);
  return UNITY_END();
}
