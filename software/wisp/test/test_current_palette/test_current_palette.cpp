#include <unity.h>

// Native-test seam: current_palette.cpp is excluded from the native src filter
// and carries no-op FreeRTOS stubs, so pulling it in directly exercises
// update() on the host.
#include "paint/current_palette.cpp"

void setUp() {}
void tearDown() {}

static wisp::Palette single(const wisp::PaletteColor& c) {
  wisp::Palette p;
  p.id = "p";
  p.colors.push_back(c);
  return p;
}

void test_float_channels_quantize_to_bytes() {
  wisp::PaletteColor c;
  c.r = 1.0f; c.g = 0.5f; c.b = 0.0f; c.w = 0.2f;
  wisp::CurrentPalette cp;
  cp.update(single(c), 0);
  TEST_ASSERT_EQUAL(1, cp.colors().size());
  const wisp::RGBW out = cp.colors()[0];
  TEST_ASSERT_EQUAL_UINT8(255, out.r);
  TEST_ASSERT_EQUAL_UINT8(128, out.g);
  TEST_ASSERT_EQUAL_UINT8(0, out.b);
  TEST_ASSERT_EQUAL_UINT8(51, out.w);
}

// An OSC sender is untrusted: out-of-range floats clamp instead of wrapping.
void test_out_of_range_channels_clamp() {
  wisp::PaletteColor c;
  c.r = 2.0f; c.g = -1.0f; c.b = 1.0001f; c.w = 0.0f;
  wisp::CurrentPalette cp;
  cp.update(single(c), 0);
  const wisp::RGBW out = cp.colors()[0];
  TEST_ASSERT_EQUAL_UINT8(255, out.r);
  TEST_ASSERT_EQUAL_UINT8(0, out.g);
  TEST_ASSERT_EQUAL_UINT8(255, out.b);
}

void test_id_and_clear() {
  wisp::PaletteColor c;
  wisp::CurrentPalette cp;
  cp.update(single(c), 42);
  TEST_ASSERT_EQUAL_STRING("p", cp.paletteId().c_str());
  TEST_ASSERT_EQUAL_UINT32(42, cp.lastChangeMs());
  cp.clear();
  TEST_ASSERT_TRUE(cp.paletteId().empty());
  TEST_ASSERT_TRUE(cp.colors().empty());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_float_channels_quantize_to_bytes);
  RUN_TEST(test_out_of_range_channels_clamp);
  RUN_TEST(test_id_and_clear);
  return UNITY_END();
}
