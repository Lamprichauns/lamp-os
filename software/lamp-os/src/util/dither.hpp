#pragma once

#include <cstdint>

namespace lamp {

// Gamma 2.6, Adafruit_NeoPixel::gamma8's curve, in 8.8 fixed point. The high
// byte is the whole 8-bit drive level and the low byte its fraction.
// Regenerate with round((i / 255) ** 2.6 * 65280).
inline constexpr uint16_t kGamma88[256] = {
        0,     0,     0,     1,     1,     2,     4,     6,     8,    11,
       14,    18,    23,    28,    34,    41,    49,    57,    66,    76,
       87,    99,   112,   125,   140,   156,   172,   190,   209,   229,
      250,   272,   296,   321,   346,   374,   402,   432,   463,   495,
      529,   564,   600,   638,   677,   718,   760,   804,   849,   896,
      944,   994,  1046,  1099,  1153,  1210,  1268,  1328,  1389,  1452,
     1517,  1584,  1652,  1722,  1794,  1868,  1944,  2021,  2100,  2182,
     2265,  2350,  2437,  2526,  2617,  2710,  2805,  2902,  3001,  3102,
     3205,  3310,  3417,  3527,  3638,  3752,  3868,  3986,  4106,  4229,
     4353,  4480,  4609,  4741,  4874,  5010,  5149,  5289,  5432,  5577,
     5725,  5875,  6027,  6182,  6340,  6499,  6661,  6826,  6993,  7163,
     7335,  7510,  7687,  7866,  8049,  8234,  8421,  8611,  8804,  8999,
     9197,  9398,  9601,  9807, 10015, 10227, 10441, 10658, 10877, 11100,
    11325, 11553, 11783, 12017, 12253, 12492, 12734, 12979, 13227, 13478,
    13731, 13988, 14247, 14509, 14775, 15043, 15314, 15588, 15866, 16146,
    16429, 16715, 17005, 17297, 17593, 17891, 18193, 18498, 18805, 19116,
    19431, 19748, 20068, 20392, 20719, 21049, 21382, 21719, 22059, 22402,
    22748, 23098, 23450, 23806, 24166, 24529, 24895, 25264, 25637, 26013,
    26393, 26776, 27162, 27552, 27945, 28341, 28741, 29145, 29552, 29962,
    30376, 30794, 31215, 31639, 32067, 32499, 32934, 33372, 33815, 34260,
    34710, 35163, 35620, 36080, 36544, 37011, 37483, 37958, 38436, 38918,
    39405, 39894, 40388, 40885, 41386, 41891, 42399, 42911, 43427, 43947,
    44471, 44998, 45530, 46065, 46604, 47147, 47693, 48244, 48798, 49357,
    49919, 50486, 51056, 51630, 52208, 52790, 53376, 53966, 54560, 55158,
    55760, 56366, 56976, 57591, 58209, 58831, 59458, 60088, 60723, 61361,
    62004, 62651, 63302, 63957, 64616, 65280};

// Drive level in 8.8 for a linear channel byte at strip level 0..255.
// Level 255 is verbatim; 0 and a zero channel are exactly 0.
inline uint16_t gammaLevel88(uint8_t value, uint8_t level) {
  return static_cast<uint16_t>((static_cast<uint32_t>(kGamma88[value]) * level + 127u) / 255u);
}

// One temporal-dither step. Emits the 8-bit drive byte and carries the
// fraction into the next frame, so the emitted bytes average to target88.
// The carry stays below one step, so a zero target emits 0 and a whole
// target emits its byte whatever the carry holds.
inline uint8_t ditherStep(uint16_t target88, uint8_t& carry) {
  const uint32_t sum = static_cast<uint32_t>(target88) + carry;
  carry = static_cast<uint8_t>(sum);
  return static_cast<uint8_t>(sum >> 8);
}

}  // namespace lamp
