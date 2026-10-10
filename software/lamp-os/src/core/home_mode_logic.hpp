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

struct HomeModeState {
  bool active = false;   // home mode is on
  bool preview = false;  // on only because the app is showing its Home Mode page
};

// Home mode from one snapshot of the BLE client state. With a client
// connected only the app's Home Mode page turns it on, as a preview;
// otherwise `fromConfig` (effectiveHomeModeFromConfig) decides.
constexpr HomeModeState homeModeState(bool clientConnected, bool homePageActive,
                                      bool fromConfig) {
  if (clientConnected) return {homePageActive, homePageActive};
  return {fromConfig, false};
}

}  // namespace lamp
