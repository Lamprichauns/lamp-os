#pragma once

#include <cstdint>
#include <cstring>

#include "util/bd_addr.hpp"

namespace lamp {

// True when a CHAR_REMOTE_OP write's top-level targetMac addresses this lamp:
// "broadcast" or this lamp's own MAC. A missing, malformed or other lamp's
// target is not applied, and nothing is forwarded.
inline bool remoteOpTargetsLamp(const char* targetMac, const uint8_t myMac[6]) {
  if (!targetMac) return false;
  if (std::strcmp(targetMac, "broadcast") == 0) return true;
  uint8_t mac[6];
  return parseBdAddr(targetMac, mac) && std::memcmp(mac, myMac, 6) == 0;
}

}  // namespace lamp
