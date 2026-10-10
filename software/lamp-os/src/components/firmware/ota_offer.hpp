#pragma once

#include <cstdint>
#include <vector>

#include "components/network/mesh/lamp_roster.hpp"

namespace lamp {

// Anything that holds the offer scan back: the compositor's startup phase, an
// OTA moving in either direction for either image, or OTA quiet mode.
struct OtaOfferBusy {
  bool startingUp = false;
  bool firmwareSending = false;
  bool firmwareReceiving = false;
  bool fsBusy = false;
  bool quiet = false;

  bool any() const {
    return startingUp || firmwareSending || firmwareReceiving || fsBusy || quiet;
  }
};

// Paces the offer scan. A busy lamp never scans and does not spend the interval.
class OtaOfferThrottle {
 public:
  // Lamps emit HELLO every 1-2 s, so 1 s catches every fresh sighting while
  // keeping the roster snapshot off the per-frame path.
  static constexpr uint32_t kIntervalMs = 1000;

  // True when a scan should run at nowMs; stamps nowMs as the last scan.
  bool due(const OtaOfferBusy& busy, uint32_t nowMs) {
    if (busy.any()) return false;
    if (lastScanMs_ != 0 &&
        static_cast<int32_t>(nowMs - lastScanMs_) < static_cast<int32_t>(kIntervalMs)) {
      return false;
    }
    lastScanMs_ = nowMs;
    return true;
  }

 private:
  uint32_t lastScanMs_ = 0;
};

// True when a reachable peer runs firmware newer than ownVersion. This lamp is
// about to receive, so anything it sent now would be re-sent under the new image.
inline bool peerRunsNewerFirmware(const std::vector<MeshPeer>& peers, uint32_t ownVersion) {
  for (const auto& p : peers) {
    if (p.hasMac && p.firmwareVersion > ownVersion) return true;
  }
  return false;
}

// Offers this lamp's firmware and filesystem image to stale peers once
// startupComplete (the compositor's startup phase has ended). Runs every loop
// on every variant, whatever the lamp's features, home mode or social mode:
// offering an update is not a social behaviour.
void tickOtaOffers(uint32_t nowMs, bool startupComplete);

}  // namespace lamp
