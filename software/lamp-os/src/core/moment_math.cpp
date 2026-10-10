#include "core/moment_math.hpp"

#include <cstdio>
#include <cstring>

#include "util/color.hpp"

namespace lamp {

namespace {

constexpr MomentPacing kPacing[kMomentKindCount] = {
    // Fireflies
    {20, 60, 30, 6, 45 * kMinuteMs, 90000},
    // Whisper
    {8, 20, 10, 6, 10 * kMinuteMs, 30000},
};

constexpr uint8_t kFirefliesOpacityLow = 10;
constexpr uint8_t kFirefliesOpacityPeak = 25;
constexpr uint16_t kFirefliesRampInBlinks = 6;
constexpr uint16_t kFirefliesRampOutFrom = 110;
constexpr uint16_t kFirefliesStarPercent = 25;

bool parseHexMac(const char* s, uint8_t out[6]) {
  for (int i = 0; i < 6; ++i) {
    if (!parseHexByte(s + 2 * i, out[i])) return false;
  }
  return true;
}

size_t appendHexMac(char* p, const uint8_t mac[6]) {
  static const char kHex[] = "0123456789ABCDEF";
  for (int i = 0; i < 6; ++i) {
    p[2 * i] = kHex[mac[i] >> 4];
    p[2 * i + 1] = kHex[mac[i] & 0x0F];
  }
  return 12;
}

}  // namespace

const MomentPacing& momentPacing(MomentKind kind) { return kPacing[momentIndex(kind)]; }

uint32_t momentModePercent(SocialMode mode) {
  switch (mode) {
    case SocialMode::Introvert: return 150;
    case SocialMode::Extrovert: return 70;
    case SocialMode::Ambivert:  return 100;
  }
  return 100;
}

uint32_t momentWaitMs(MomentKind kind, uint8_t growth, SocialMode mode, FastRng& rng) {
  const MomentPacing& p = momentPacing(kind);
  const uint32_t g = growth > p.growthCap ? p.growthCap : growth;
  const uint32_t shift = g * p.stepMin;
  const uint32_t ms = rng.range((p.loMin + shift) * kMinuteMs, (p.hiMin + shift) * kMinuteMs);
  return static_cast<uint32_t>(static_cast<uint64_t>(ms) * momentModePercent(mode) / 100);
}

uint8_t momentGrow(MomentKind kind, uint8_t growth) {
  const uint8_t cap = momentPacing(kind).growthCap;
  return growth >= cap ? cap : static_cast<uint8_t>(growth + 1);
}

uint32_t momentBackoffMs(MomentKind kind, FastRng& rng) {
  return rng.range(0, momentPacing(kind).backoffMaxMs);
}

uint32_t momentHash(uint32_t x) {
  x ^= x >> 16;
  x *= 0x85EBCA6Bu;
  x ^= x >> 13;
  x *= 0xC2B2AE35u;
  x ^= x >> 16;
  return x;
}

uint32_t momentHashMac(const uint8_t mac[6]) {
  uint32_t h = 0x811C9DC5u;
  for (int i = 0; i < 6; ++i) {
    h ^= mac[i];
    h *= 0x01000193u;
  }
  return momentHash(h);
}

uint32_t macLow32(const uint8_t mac[6]) {
  return (static_cast<uint32_t>(mac[2]) << 24) | (static_cast<uint32_t>(mac[3]) << 16) |
         (static_cast<uint32_t>(mac[4]) << 8) | mac[5];
}

uint32_t firefliesPeriodMs(uint32_t seed) { return 1000 + seed % 600; }

int32_t firefliesFirstOffset(uint32_t seed, const uint8_t mac[6]) {
  const uint32_t t = firefliesPeriodMs(seed);
  return static_cast<int32_t>(momentHash(seed ^ macLow32(mac)) % t) -
         static_cast<int32_t>(t / 2);
}

// Integer rounding keeps every lamp's d_k identical to the native vectors,
// whatever its float unit does.
int32_t firefliesNextOffset(int32_t offset, uint16_t blink) {
  if (blink <= kFirefliesScatterBlinks) return offset;
  const int64_t scaled = static_cast<int64_t>(offset) * (1000 - kFirefliesNudgePermille);
  const int32_t next = static_cast<int32_t>((scaled + (scaled >= 0 ? 500 : -500)) / 1000);
  return (next > -kFirefliesSnapMs && next < kFirefliesSnapMs) ? 0 : next;
}

uint8_t firefliesOpacity(uint16_t blink) {
  constexpr uint32_t span = kFirefliesOpacityPeak - kFirefliesOpacityLow;
  if (blink <= kFirefliesRampInBlinks) {
    const uint32_t step = blink == 0 ? 0 : blink - 1;
    return static_cast<uint8_t>(kFirefliesOpacityLow +
                                span * step / (kFirefliesRampInBlinks - 1));
  }
  if (blink <= kFirefliesRampOutFrom) return kFirefliesOpacityPeak;
  const uint32_t out = blink >= kFirefliesBlinks ? kFirefliesBlinks - kFirefliesRampOutFrom
                                                 : blink - kFirefliesRampOutFrom;
  return static_cast<uint8_t>(kFirefliesOpacityPeak -
                              span * out / (kFirefliesBlinks - kFirefliesRampOutFrom));
}

uint16_t firefliesStars(uint16_t surfacePixels) {
  const uint32_t stars = static_cast<uint32_t>(surfacePixels) * kFirefliesStarPercent / 100;
  return static_cast<uint16_t>(stars < 1 ? 1 : stars);
}

bool macIn(const uint8_t mac[6], const uint8_t (*list)[6], uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    if (std::memcmp(mac, list[i], 6) == 0) return true;
  }
  return false;
}

int compareMac(const uint8_t a[6], const uint8_t b[6]) { return std::memcmp(a, b, 6); }

size_t encodeMomentFrame(const MomentFrame& f, char* out, size_t cap) {
  char buf[256];
  int n;
  if (f.kind == MomentKind::Whisper) {
    char to[13] = {0};
    appendHexMac(to, f.to);
    char visited[kWhisperMaxHops * 12 + 1] = {0};
    const uint8_t hops = f.hop > kWhisperMaxHops ? kWhisperMaxHops : f.hop;
    for (uint8_t i = 0; i < hops; ++i) appendHexMac(visited + 12 * i, f.visited[i]);
    char color[16] = {0};
    if (f.hasColor) {
      std::snprintf(color, sizeof(color), ",\"c\":\"%02x%02x%02x%02x\"", f.color.r, f.color.g,
                    f.color.b, f.color.w);
    }
    n = std::snprintf(buf, sizeof(buf),
                      "{\"type\":\"moment\",\"mo\":{\"k\":%u,\"s\":%lu,\"h\":%u,\"a\":%u,"
                      "\"to\":\"%s\",\"v\":\"%s\"%s}}",
                      static_cast<unsigned>(f.kind), static_cast<unsigned long>(f.seed),
                      static_cast<unsigned>(hops), static_cast<unsigned>(f.ageMs), to,
                      visited, color);
  } else {
    n = std::snprintf(buf, sizeof(buf), "{\"type\":\"moment\",\"mo\":{\"k\":%u,\"s\":%lu,\"a\":%u}}",
                      static_cast<unsigned>(f.kind), static_cast<unsigned long>(f.seed),
                      static_cast<unsigned>(f.ageMs));
  }
  if (n <= 0 || static_cast<size_t>(n) >= sizeof(buf) || static_cast<size_t>(n) > cap) return 0;
  std::memcpy(out, buf, static_cast<size_t>(n));
  return static_cast<size_t>(n);
}

bool decodeMomentFrame(JsonObjectConst mo, MomentFrame& out) {
  if (!mo["k"].is<uint8_t>() || !mo["s"].is<uint32_t>() || !mo["a"].is<uint32_t>()) {
    return false;
  }
  const uint8_t k = mo["k"].as<uint8_t>();
  const uint32_t age = mo["a"].as<uint32_t>();
  if (age > kMomentMaxAgeMs) return false;
  MomentFrame f;
  f.seed = mo["s"].as<uint32_t>();
  f.ageMs = static_cast<uint16_t>(age);
  if (k == static_cast<uint8_t>(MomentKind::Fireflies)) {
    f.kind = MomentKind::Fireflies;
    out = f;
    return true;
  }
  if (k != static_cast<uint8_t>(MomentKind::Whisper)) return false;
  f.kind = MomentKind::Whisper;
  if (!mo["h"].is<uint8_t>()) return false;
  const uint8_t h = mo["h"].as<uint8_t>();
  if (h < 1 || h > kWhisperMaxHops) return false;
  const char* to = mo["to"].as<const char*>();
  const char* v = mo["v"].as<const char*>();
  if (!to || !v || std::strlen(to) != 12 || std::strlen(v) != 12u * h) return false;
  if (!parseHexMac(to, f.to)) return false;
  for (uint8_t i = 0; i < h; ++i) {
    if (!parseHexMac(v + 12 * i, f.visited[i])) return false;
  }
  if (!mo["c"].isNull()) {
    const char* c = mo["c"].as<const char*>();
    uint8_t rgbw[4];
    if (!c || std::strlen(c) != 8) return false;
    for (int i = 0; i < 4; ++i) {
      if (!parseHexByte(c + 2 * i, rgbw[i])) return false;
    }
    f.hasColor = true;
    f.color = Color(rgbw[0], rgbw[1], rgbw[2], rgbw[3]);
  }
  f.hop = h;
  out = f;
  return true;
}

}  // namespace lamp
