#pragma once

#include <cstdint>

namespace webapp {

// Boot-window expiry decision, inline so it's unit-testable without WiFi.
// `neverExpire` short-circuits (apBootMinutes==0 keeps the AP up); the finite
// path is millis()-wrap-safe via signed difference.
inline bool webappShouldTeardown(uint32_t now, uint32_t deadlineMs,
                                 bool neverExpire) {
  return !neverExpire && static_cast<int32_t>(now - deadlineMs) >= 0;
}

// BLE was torn down for a serve and the session has gone idle (HTTP/ws
// deadline elapsed). Ignores station count + never-expire so a closed
// browser that stays associated still gets BLE back via reboot.
inline bool webappShouldRestoreBle(uint32_t now, uint32_t deadlineMs,
                                   bool bleDown) {
  return bleDown && static_cast<int32_t>(now - deadlineMs) >= 0;
}

// An armed reboot (0 = none) is due once its time passes, whether or not the
// AP is still up, and waits while an OTA holds quiet mode.
inline bool webappRebootDue(uint32_t now, uint32_t rebootAtMs, bool otaQuiet) {
  return rebootAtMs != 0 && static_cast<int32_t>(now - rebootAtMs) >= 0 && !otaQuiet;
}

}  // namespace webapp
