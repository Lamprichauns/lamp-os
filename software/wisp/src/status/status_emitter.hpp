// StatusEmitter drives the wispStatus heartbeat on a 30s FreeRTOS timer.
//
// Broadcasts MSG_CONTROL_OP (wispStatus JSON) then MSG_WISP_PALETTE. The timer
// (and triggerOnChange) only flag the emit due; pump() runs the build +
// broadcast on the loop task so nothing heavy touches the 2KB timer stack.
// pump() also detects wifi connectivity edges and flags an emit for them;
// the controller flags source-liveness edges itself. Emits are coalesced to
// one per 5 s minimum: a due flag inside the window is deferred until the
// window opens, never dropped. wispConfig follows on the cadence ConfigCadence
// sets, always in a separate pump pass from the status.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "status/config_cadence.hpp"
#include "status/status_json.hpp"
#include "wire/lamp_protocol.hpp"

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
using StatusEmitterTimerHandle = TimerHandle_t;
#else
using StatusEmitterTimerHandle = void*;
#endif

namespace wisp {

class CurrentPalette;
class MeshLink;
class WifiLink;
class WispConfig;
class WispController;
struct SeqSource;

class StatusEmitter {
 public:
  void begin(MeshLink* mesh, const WispController* controller,
             WispConfig* config, CurrentPalette* palette, SeqSource* seq,
             WifiLink* wifi);

  // One-shot wiring of the 30s heartbeat timer. Call once after begin().
  void startTimer();

  // Mark a wispStatus emit due AND reschedule the heartbeat from now. Call
  // after applying an op that changes reported state so the app sees it
  // without waiting up to 30s. The emit runs from pump() on the loop task
  // (subject to the 5 s coalescer), so this is safe from any task and does
  // no heavy work itself.
  void triggerOnChange();

  // Mark a wispConfig emit due (pollConfig). Safe from any task; the emit
  // runs from pump(), coalesced by ConfigCadence.
  void pollConfig();

  // Runs the due wispStatus / wispConfig emit on the loop task. Cheap when
  // nothing is due. True when it ran a wispStatus or wispConfig emit this pass.
  bool pump();

 private:
  // wifi connectivity flips flag an emit. Loop task only.
  void checkConnEdges();

  // Emit bodies run only from pump() on the loop task.
  void emitStatus();   // MSG_CONTROL_OP wispStatus path (30s heartbeat)
  void emitPalette();  // MSG_WISP_PALETTE (piggybacked on emitStatus)
  void emitConfig();   // MSG_CONTROL_OP wispConfig path

  WispConfigFields configFields() const;
  size_t buildConfig(char* out, size_t outCap) const;
  void broadcastControlOp(const char* json, size_t jsonLen, const char* what);

  MeshLink* mesh_ = nullptr;
  const WispController* controller_ = nullptr;
  WispConfig* config_ = nullptr;
  CurrentPalette* palette_ = nullptr;
  SeqSource* seq_ = nullptr;
  WifiLink* wifi_ = nullptr;
  StatusEmitterTimerHandle statusTimer_ = nullptr;

  // Set from the timer task / triggerOnChange, cleared in pump() on the loop task.
  std::atomic<bool> statusDue_{false};
  std::atomic<bool> configPollDue_{false};

  bool lastWifiConnected_ = false;
  bool lastEthUp_ = false;
  bool haveLastConnState_ = false;

  uint32_t lastEmitMs_ = 0;
  bool haveEmitted_    = false;

  // After a state change the status re-broadcasts at kStatusBurstIntervalMs
  // until kStatusBurstMs elapses, so the confirmation survives a multi-second
  // coex reception burst on the lamps.
  uint32_t burstUntilMs_ = 0;

  ConfigCadence configCadence_;

  static constexpr uint32_t kStatusIntervalMs = 30000;
  static constexpr uint32_t kMinEmitIntervalMs = 5000;
  static constexpr uint32_t kStatusBurstMs = 20000;
  static constexpr uint32_t kStatusBurstIntervalMs = 2000;
  static constexpr size_t kStatusJsonBufLen =
      lamp_protocol::CONTROL_MAX_PAYLOAD + 1;
  static constexpr size_t kConfigJsonBufLen = 256;
};

}  // namespace wisp
