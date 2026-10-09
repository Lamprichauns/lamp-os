// Which Straylight palette role the wisp follows in Osc mode. The role names
// are the wire strings for wispOp setOscRole, the console and wispStatus.

#pragma once

#include <cstdint>
#include <cstring>

namespace wisp {

// NVS u8: 0=Primary, 1=Secondary, 2=Tertiary, 3=Fill.
enum class OscRole : uint8_t {
  Primary   = 0,
  Secondary = 1,
  Tertiary  = 2,
  Fill      = 3,
};

inline constexpr uint8_t kOscRoleCount = 4;

inline constexpr const char* kOscRoleNames[kOscRoleCount] = {
    "primary", "secondary", "tertiary", "fill"};

// The only place a listened address is built. Layout is flat RGB floats 0..1.
inline constexpr const char* kOscPaletteAddresses[kOscRoleCount] = {
    "/straylight/palette/primary/colours",
    "/straylight/palette/secondary/colours",
    "/straylight/palette/tertiary/colours",
    "/straylight/palette/fill/colours",
};

// Unknown raw values (a corrupted NVS byte) read as Primary.
inline OscRole coerceOscRole(int raw) {
  return raw >= 0 && raw < kOscRoleCount ? static_cast<OscRole>(raw)
                                         : OscRole::Primary;
}

inline uint8_t oscRoleIndex(OscRole r) {
  return static_cast<uint8_t>(coerceOscRole(static_cast<int>(r)));
}

inline const char* oscRoleName(OscRole r) {
  return kOscRoleNames[oscRoleIndex(r)];
}

inline const char* oscPaletteAddress(OscRole r) {
  return kOscPaletteAddresses[oscRoleIndex(r)];
}

// False when `name` is null or not a role name.
inline bool oscRoleFromName(const char* name, OscRole& out) {
  if (!name) return false;
  for (uint8_t i = 0; i < kOscRoleCount; ++i) {
    if (std::strcmp(name, kOscRoleNames[i]) == 0) {
      out = static_cast<OscRole>(i);
      return true;
    }
  }
  return false;
}

}  // namespace wisp
