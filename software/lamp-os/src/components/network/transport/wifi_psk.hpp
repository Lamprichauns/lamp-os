#pragma once

#include <cstddef>

namespace wifi {

// WPA2-PSK keys off an 8..63 character passphrase. A lamp password outside
// that range cannot key the softAP, which then comes up as an open network.
constexpr size_t kPskMinLen = 8;
constexpr size_t kPskMaxLen = 63;

inline bool pskUsable(size_t len) {
  return len >= kPskMinLen && len <= kPskMaxLen;
}

}  // namespace wifi
