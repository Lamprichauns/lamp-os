// PresenceBeacon broadcasts wisp presence on a 2s FreeRTOS timer.
//
// MSG_WISP_HELLO fires every 2s tick, with the fat full-set MSG_WISP_CLAIM on
// odd ticks. The fat full-set MSG_WISP_STATE goes out every tick at a random
// offset after the HELLO. STATE is the sole paint render path (per-lamp
// colors), so a paint-mode edge also emits STATE out of cadence via
// PaintDistributor's dirty flag, then re-emits it a few times over a short
// burst window so the take/release edge survives a coex drop instead of waiting
// on the 2s re-cover. A WiFi/source-live flag flip triggers
// StatusEmitter's on-change path so the app sees it within 2s rather than
// waiting on the 30s heartbeat. The timer only flags the emit due; pump() runs
// the build + broadcast on the loop task, off the 2KB timer stack.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "version.hpp"
#include "wire/lamp_protocol.hpp"

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
using PresenceBeaconTimerHandle = TimerHandle_t;
#else
using PresenceBeaconTimerHandle = void*;
#endif

namespace wisp {

class CurrentPalette;
class MeshLink;
class PaintDistributor;
class StatusEmitter;
class WispConfig;
class WispController;
class WispRoster;
struct SeqSource;

// True when the STATE burst window (opened on a paint-mode edge) is still open
// at `now` and at least intervalMs has elapsed since the last STATE emit, so an
// extra STATE copy is due. burstUntilMs == 0 (no window) or a closed/too-soon
// window returns false. Pure so the burst timing is unit-testable.
inline bool stateBurstDue(uint32_t now, uint32_t burstUntilMs,
                          uint32_t lastStateEmitMs, uint32_t intervalMs) {
  if (burstUntilMs == 0) return false;
  if (static_cast<int32_t>(now - burstUntilMs) >= 0) return false;
  return (now - lastStateEmitMs) >= intervalMs;
}

// A fixed STATE phase behind the HELLO beats against a lamp's periodic deaf
// window and loses whole runs of STATEs; a per-tick random offset spreads them.
constexpr uint32_t kStateOffsetMinMs = 200;
constexpr uint32_t kStateOffsetMaxMs = 800;

// Maps a raw random word to the tick STATE's delay after the HELLO, uniform
// over [kStateOffsetMinMs, kStateOffsetMaxMs].
inline uint32_t stateTickOffsetMs(uint32_t random) {
  return kStateOffsetMinMs + random % (kStateOffsetMaxMs - kStateOffsetMinMs + 1);
}

// The steady-cadence STATE each HELLO tick schedules. It holds one pending
// STATE, and a schedule() replaces an unsent one, so a loop stall drops at most
// that STATE.
class TickStateSchedule {
 public:
  void schedule(uint32_t helloMs, uint32_t offsetMs) {
    pending_ = true;
    dueMs_ = helloMs + offsetMs;
  }
  // True from the due time until markSent().
  bool due(uint32_t now) const {
    return pending_ && static_cast<int32_t>(now - dueMs_) >= 0;
  }
  void markSent() { pending_ = false; }

 private:
  bool     pending_ = false;
  uint32_t dueMs_   = 0;
};

enum class StateSend : uint8_t { None, Edge, Burst, Tick };

// Which STATE a loop pass sends. None on a pass that already broadcast another
// fat frame, else the paint edge, then a due burst copy, then a due tick STATE.
constexpr StateSend nextStateSend(bool passBusy, bool edgeDirty, bool burstDue,
                                  bool tickDue) {
  if (passBusy) return StateSend::None;
  if (edgeDirty) return StateSend::Edge;
  if (burstDue) return StateSend::Burst;
  if (tickDue) return StateSend::Tick;
  return StateSend::None;
}

class PresenceBeacon {
 public:
  void begin(MeshLink* mesh, PaintDistributor* paint, CurrentPalette* palette,
             const WispController* controller, WispRoster* roster,
             SeqSource* seq,
             StatusEmitter* statusEmitter, WispConfig* config);

  // Call once after begin().
  void startTimer();

  // Runs the due HELLO / CLAIM / STATE emits on the loop task. Pass true when
  // another emitter already broadcast a fat frame this loop pass. Any STATE,
  // edge and burst copies included, waits for a pass with no other fat frame
  // (nextStateSend), so two fat frames never go out in one pass on the single
  // core. Cheap when nothing is due.
  void pump(bool fatFrameSentThisPass);

 private:
  void emit();                              // HELLO every 2s + claim on odd ticks
  void emitClaim(const uint8_t srcMac[6]);  // full-set MSG_WISP_CLAIM
  // full-set MSG_WISP_STATE; presenceFlags carries the same WISP_HELLO_FLAG_*
  // bits computed for the HELLO this tick. Packs color entries only while
  // painting, so Off or a stale source broadcasts an empty set and lamps
  // ease home.
  void emitState(const uint8_t srcMac[6], uint8_t presenceFlags);
  // Out-of-cadence STATE for a paint-mode edge; recomputes mac + flags.
  void emitStateEvent();
  uint8_t computePresenceFlags(bool wifiNow, bool sourceLiveNow) const;

  // Re-broadcast a just-sent frame (same seq) `copies` more times, kResendGapMs
  // apart: an unacked broadcast has no retry, so a receiver whose scan-listen
  // gap swallows one copy still catches another. Dedup collapses them to one.
  void queueResend(const uint8_t* frame, size_t len, uint8_t copies, uint32_t now);
  // True when it broadcast a resend copy.
  bool serviceResends(uint32_t now);

  MeshLink* mesh_ = nullptr;
  PaintDistributor* paint_ = nullptr;
  CurrentPalette* palette_ = nullptr;
  const WispController* controller_ = nullptr;
  WispRoster* roster_ = nullptr;
  SeqSource* seq_ = nullptr;
  StatusEmitter* statusEmitter_ = nullptr;
  WispConfig* config_ = nullptr;
  PresenceBeaconTimerHandle timer_ = nullptr;

  // Set from the timer task, cleared in pump() on the loop task.
  std::atomic<bool> helloDue_{false};

  bool lastHelloWifi_       = false;
  bool lastHelloSourceLive_ = false;
  bool haveLastHelloConn_   = false;

  // 2s-tick counter driving the odd-tick claim.
  uint32_t beaconTick_ = 0;

  // The tick STATE scheduled by emit(), sent from a later pump() pass.
  TickStateSchedule tickState_;

  // Elevated-rate STATE window opened on a paint-mode edge. 0 when closed.
  uint32_t stateBurstUntilMs_ = 0;
  uint32_t lastStateEmitMs_   = 0;

  struct ResendSlot {
    // CLAIM is the largest resent frame; STATE is not resent (too big, and it
    // re-covers on its own cadence).
    uint8_t  frame[lamp_protocol::WISP_CLAIM_MAX_SIZE];
    uint16_t len = 0;
    uint8_t  remaining = 0;
    uint32_t nextMs = 0;
  };
  // HELLO + at most one CLAIM per tick, so 3 slots never collide before the
  // resends drain well within the 2s cadence.
  static constexpr size_t   kResendSlots  = 3;
  static constexpr uint8_t  kHelloResends = 2;
  static constexpr uint8_t  kFatResends   = 1;
  static constexpr uint32_t kResendGapMs  = 40;
  ResendSlot resends_[kResendSlots]{};
  size_t     resendCursor_ = 0;

  static constexpr uint32_t kHelloIntervalMs = 2000;
  // STATE edge-burst: emit a few extra copies over kStateBurstMs, spaced
  // kStateBurstIntervalMs apart. Too short/sparse and a take/release edge lost
  // to coex waits the full 2s re-cover; too long/dense wastes air against the
  // tick fat frames on the single core.
  static constexpr uint32_t kStateBurstMs         = 2000;
  static constexpr uint32_t kStateBurstIntervalMs = 700;
  // Follows the root VERSION file (injected as LAMP_FW_* by inject_version.py).
  static constexpr uint32_t kWispVersion     = FIRMWARE_VERSION;
};

}  // namespace wisp
