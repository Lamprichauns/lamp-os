#pragma once

#include <cstdint>

namespace lamp {

// The firmware distributor's inter-session quiet hold. During a fleet OTA wave
// several below-version peers queue; holding quiet for kHoldMs after a session
// ends lets the next offer arm inside the hold and inherit its quiet, so the
// indicator stays up instead of flickering back to normal between sessions.
// The caller serializes access (the distributor's state mux).
struct InterSessionQuietHold {
  static constexpr uint32_t kHoldMs = 10000;

  bool held = false;      // this distributor owns one ota_quiet_mode count
  uint32_t exitAtMs = 0;  // 0 while a session runs or nothing is pending
  bool endedDone = false;

  void sessionEnteredQuiet() {
    held = true;
    exitAtMs = 0;
  }

  // Inert unless held: a session that never entered quiet leaves nothing to hold.
  void sessionEnded(uint32_t nowMs, bool done) {
    exitAtMs = nowMs + kHoldMs;
    endedDone = done;
  }

  void offerArmed() { exitAtMs = 0; }

  bool dueToExit(uint32_t nowMs) const { return held && exitAtMs != 0 && nowMs >= exitAtMs; }

  // Call after the matching exitQuiet.
  void release() {
    held = false;
    exitAtMs = 0;
  }

  // True while idle inside a hold armed by a session that ended Done: the only
  // hold a new offer may arm inside. A failed session's hold just expires, so a
  // crowd of peers that never accept cannot keep the lamp quiet indefinitely.
  bool inheritable() const { return held && exitAtMs != 0 && endedDone; }
};

}  // namespace lamp
