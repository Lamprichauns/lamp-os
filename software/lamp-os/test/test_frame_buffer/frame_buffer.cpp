// Native tests for FrameBuffer multi-segment fanout.
//
// Adafruit_NeoPixel.h is stubbed by test/native_stubs/Adafruit_NeoPixel.h,
// found via -I test/native_stubs in the native env build_flags.

#include <unity.h>

#include <cstdint>
#include <vector>

#include "../../src/core/frame_buffer.cpp"

using namespace lamp;

static Adafruit_NeoPixel neo0;
static Adafruit_NeoPixel neo1;

void setUp() {
  neo0 = Adafruit_NeoPixel{};
  neo1 = Adafruit_NeoPixel{};
}
void tearDown() {}

static uint32_t encode(const Color& c) {
  return (uint32_t)((Adafruit_NeoPixel::gamma8(c.w) << 24) |
                    (Adafruit_NeoPixel::gamma8(c.r) << 16) |
                    (Adafruit_NeoPixel::gamma8(c.g) << 8) |
                    Adafruit_NeoPixel::gamma8(c.b));
}

// New API and old adapter produce identical setPixelColor call sequences.
void test_new_api_and_adapter_flush_identically() {
  const Color red(0xFF, 0, 0, 0);
  const Color blue(0, 0, 0xFF, 0);

  FrameBuffer fb1;
  fb1.begin({red, blue, red}, std::vector<StripSegment>{{&neo0, "test", 0, 3}});
  fb1.buffer = {red, blue, red};
  neo0.reset();
  fb1.flush();
  auto calls1 = neo0.pixelCalls;

  FrameBuffer fb2;
  fb2.begin({red, blue, red}, 3, &neo0);
  fb2.buffer = {red, blue, red};
  neo0.reset();
  fb2.flush();
  auto calls2 = neo0.pixelCalls;

  TEST_ASSERT_EQUAL_UINT(calls1.size(), calls2.size());
  for (size_t i = 0; i < calls1.size(); i++) {
    TEST_ASSERT_EQUAL_UINT16(calls1[i].n, calls2[i].n);
    TEST_ASSERT_EQUAL_UINT32(calls1[i].c, calls2[i].c);
  }
}

// 2-segment buffer routes buffer[0..a) to seg0, buffer[a..a+b) to seg1.
void test_multisegment_routes_pixels_to_correct_drivers() {
  const uint16_t a = 3, b = 4;
  const Color red(0xFF, 0, 0, 0);
  const Color green(0, 0xFF, 0, 0);

  FrameBuffer fb;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "s0", 0, a}, {&neo1, "s1", a, b}});

  TEST_ASSERT_EQUAL_UINT8(a + b, fb.pixelCount);

  for (uint16_t i = 0; i < a; i++) fb.buffer[i] = red;
  for (uint16_t i = 0; i < b; i++) fb.buffer[a + i] = green;

  fb.flush();

  TEST_ASSERT_EQUAL_UINT(a, neo0.pixelCalls.size());
  for (size_t i = 0; i < neo0.pixelCalls.size(); i++) {
    TEST_ASSERT_EQUAL_UINT16(i, neo0.pixelCalls[i].n);
    TEST_ASSERT_EQUAL_UINT32(encode(red), neo0.pixelCalls[i].c);
  }

  TEST_ASSERT_EQUAL_UINT(b, neo1.pixelCalls.size());
  for (size_t i = 0; i < neo1.pixelCalls.size(); i++) {
    TEST_ASSERT_EQUAL_UINT16(i, neo1.pixelCalls[i].n);
    TEST_ASSERT_EQUAL_UINT32(encode(green), neo1.pixelCalls[i].c);
  }
}

// canShow() false on one segment: show() skipped and previousBuffer not advanced,
// so the next flush retransmits rather than short-circuiting.
void test_skipped_segment_leaves_previous_buffer_unadvanced() {
  const Color red(0xFF, 0, 0, 0);
  const Color blue(0, 0, 0xFF, 0);

  FrameBuffer fb;
  neo0.canShowResult = true;
  neo1.canShowResult = true;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "s0", 0, 2}, {&neo1, "s1", 2, 2}});

  // Establish a fully-shown baseline.
  fb.buffer = {red, red, red, red};
  fb.flush();
  TEST_ASSERT_TRUE(fb.previousBuffer == fb.buffer);

  // Block neo1 and change the buffer.
  neo1.canShowResult = false;
  fb.buffer = {blue, blue, blue, blue};
  neo0.reset();
  neo1.reset();
  fb.flush();

  TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
  TEST_ASSERT_EQUAL_INT(0, neo1.showCount);
  // previousBuffer stays at the last fully-shown state, not the current buffer.
  TEST_ASSERT_FALSE(fb.previousBuffer == fb.buffer);

  // Next flush with the same buffer: not short-circuited (previousBuffer still old).
  neo0.reset();
  neo1.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_UINT(2, neo0.pixelCalls.size());
  TEST_ASSERT_EQUAL_UINT(2, neo1.pixelCalls.size());
  TEST_ASSERT_EQUAL_INT(0, neo1.showCount);

  // Unblock neo1: full flush advances previousBuffer.
  neo1.canShowResult = true;
  neo0.reset();
  neo1.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_INT(1, neo1.showCount);
  TEST_ASSERT_TRUE(fb.previousBuffer == fb.buffer);
}

// A strip-level change alone (the governor's pre-flush clamp) defeats the
// content dedup: every pixel is rewritten and shown at the new level.
void test_level_change_forces_full_repush() {
  const Color red(0xFF, 0, 0, 0);

  FrameBuffer fb;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "s0", 0, 3}});
  fb.buffer = {red, red, red};
  fb.flush();
  TEST_ASSERT_EQUAL_UINT8(255, fb.previousLevel);

  // Same content, same level: dedup short-circuits.
  neo0.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_UINT(0, neo0.pixelCalls.size());
  TEST_ASSERT_EQUAL_INT(0, neo0.showCount);

  // Same content, clamped level: full repush at the folded-in level.
  fb.level = 127;
  neo0.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_UINT(3, neo0.pixelCalls.size());
  TEST_ASSERT_EQUAL_UINT32(127u << 16, neo0.pixelCalls[0].c);
  TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
  TEST_ASSERT_EQUAL_UINT8(127, fb.previousLevel);
}

static uint8_t redOf(const Adafruit_NeoPixel::PixelCall& call) { return (call.c >> 16) & 0xFF; }

// A changing pixel between drive levels flushes every frame and averages to its
// 8.8 target, each pixel on its own carry and staggered against its neighbours.
// Toggling w between 0 and 1 changes the colour while w's drive stays 0.
void test_changing_pixels_dither_independently_and_staggered() {
  const uint16_t n = 4;
  FrameBuffer fb;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "s0", 0, n}});
  TEST_ASSERT_EQUAL_UINT16(896, kGamma88[49]);
  uint32_t redSum[n] = {};
  for (int f = 0; f < 256; f++) {
    fb.buffer.assign(n, Color(49, 0, 0, f & 1));
    neo0.reset();
    fb.flush();
    TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
    bool allEqual = true;
    for (uint16_t i = 0; i < n; i++) {
      TEST_ASSERT_EQUAL_UINT32(0, neo0.pixelCalls[i].c & 0xFF00FFFFu);
      redSum[i] += redOf(neo0.pixelCalls[i]);
      allEqual &= redOf(neo0.pixelCalls[i]) == redOf(neo0.pixelCalls[0]);
    }
    TEST_ASSERT_FALSE(allEqual);
  }
  for (uint16_t i = 0; i < n; i++) TEST_ASSERT_EQUAL_UINT32(896, redSum[i]);
}

// A pixel held still dithers through the hold, then shows its rounded level
// and the dedup stops pushing; a new colour resumes the push.
void test_static_pixel_settles_to_rounded_level_and_dedups() {
  FrameBuffer fb;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "s0", 0, 1}});
  fb.buffer = {Color(49, 0, 0, 0)};  // 3.5 drive levels, rounds to 4
  bool sawThree = false;
  bool sawFour = false;
  for (int f = 0; f < FrameBuffer::kDitherHoldFrames; f++) {
    neo0.reset();
    fb.flush();
    TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
    sawThree |= redOf(neo0.pixelCalls[0]) == 3;
    sawFour |= redOf(neo0.pixelCalls[0]) == 4;
  }
  TEST_ASSERT_TRUE(sawThree && sawFour);

  neo0.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
  TEST_ASSERT_EQUAL_UINT32(4u << 16, neo0.pixelCalls[0].c);
  for (int f = 0; f < 50; f++) {
    neo0.reset();
    fb.flush();
    TEST_ASSERT_EQUAL_INT(0, neo0.showCount);
  }

  fb.buffer = {Color(50, 0, 0, 0)};
  neo0.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
  neo0.reset();
  fb.flush();
  TEST_ASSERT_EQUAL_INT(1, neo0.showCount);
}

// A fade to black is exactly off from the frame it reaches zero, and full stays
// exact, through the fade, the hold and after it.
void test_zero_stays_off_and_full_stays_exact() {
  FrameBuffer fb;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "s0", 0, 2}});
  const Color full(255, 255, 255, 255);
  for (int v = 60; v >= -40; v--) {
    fb.buffer = {Color(v > 0 ? v : 0, 0, 0, 0), full};
    neo0.reset();
    fb.flush();
    if (neo0.pixelCalls.empty()) continue;
    if (v <= 0) TEST_ASSERT_EQUAL_UINT32(0, neo0.pixelCalls[0].c);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, neo0.pixelCalls[1].c);
  }
  TEST_ASSERT_TRUE(neo0.pixelCalls.empty());
}

// reversed=true flips pixel order; reversed=false is byte-identical to before.
void test_reversed_segment_flips_pixel_order() {
  const Color c0(0xFF, 0,    0,    0);
  const Color c1(0,    0xFF, 0,    0);
  const Color c2(0,    0,    0xFF, 0);
  const Color c3(0,    0,    0,    0xFF);
  const std::vector<Color> pattern = {c0, c1, c2, c3};

  // reversed segment: driver pixel i should receive buffer[offset + (3-i)]
  FrameBuffer fwd;
  fwd.begin({}, std::vector<StripSegment>{{&neo0, "rev", 0, 4, true}});
  fwd.buffer = pattern;
  neo0.reset();
  fwd.flush();
  TEST_ASSERT_EQUAL_UINT(4, neo0.pixelCalls.size());
  for (size_t i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL_UINT16(i, neo0.pixelCalls[i].n);
    TEST_ASSERT_EQUAL_UINT32(encode(pattern[3 - i]), neo0.pixelCalls[i].c);
  }

  // non-reversed segment: driver pixel i should receive buffer[offset + i]
  FrameBuffer fwd2;
  fwd2.begin({}, std::vector<StripSegment>{{&neo0, "norm", 0, 4, false}});
  fwd2.buffer = pattern;
  neo0.reset();
  fwd2.flush();
  TEST_ASSERT_EQUAL_UINT(4, neo0.pixelCalls.size());
  for (size_t i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL_UINT16(i, neo0.pixelCalls[i].n);
    TEST_ASSERT_EQUAL_UINT32(encode(pattern[i]), neo0.pixelCalls[i].c);
  }
}

int main(int argc, char** argv) {
  (void)argc; (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_new_api_and_adapter_flush_identically);
  RUN_TEST(test_multisegment_routes_pixels_to_correct_drivers);
  RUN_TEST(test_skipped_segment_leaves_previous_buffer_unadvanced);
  RUN_TEST(test_level_change_forces_full_repush);
  RUN_TEST(test_changing_pixels_dither_independently_and_staggered);
  RUN_TEST(test_static_pixel_settles_to_rounded_level_and_dedups);
  RUN_TEST(test_zero_stays_off_and_full_stays_exact);
  RUN_TEST(test_reversed_segment_flips_pixel_order);
  return UNITY_END();
}
