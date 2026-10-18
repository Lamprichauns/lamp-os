#pragma once

#include <ArduinoJson.h>

#include <cstdint>

namespace lamp {
namespace apply {

// Absent or invalid keys leave the field alone, so a partial blob from a client
// that never sends them can't reset what another client set.
template <typename Surface>
inline void colorHarmonyLocal(JsonObjectConst section, Surface& surf) {
  if (section["autoColors"].is<bool>()) {
    surf.autoColors = section["autoColors"].as<bool>();
  }
  if (section["colorScheme"].is<int>()) {
    const int scheme = section["colorScheme"].as<int>();
    if (scheme >= 0 && scheme <= 255) {
      surf.colorScheme = static_cast<uint8_t>(scheme);
    }
  }
}

}  // namespace apply
}  // namespace lamp
