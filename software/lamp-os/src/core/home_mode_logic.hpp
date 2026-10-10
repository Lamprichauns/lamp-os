#pragma once

namespace lamp {

// Pure predicate for the no-BT-client home-mode decision.
// Extracted for native testability (no BLE/WiFi deps).
inline bool effectiveHomeModeFromConfig(bool enabled, bool networkBound,
                                        bool ssidEmpty, bool ssidVisible) {
  if (!enabled) return false;
  if (!networkBound) return true;
  return !ssidEmpty && ssidVisible;
}

// True when home mode is on only because the app is connected and showing the
// Home Mode page: with a client connected, home mode is on for nothing else.
constexpr bool homeModePreview(bool clientConnected, bool homeActive) {
  return clientConnected && homeActive;
}

}  // namespace lamp
