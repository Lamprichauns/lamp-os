// Decides when the wisp's WiFi STA scans for, joins and drops its stored
// network. The caller feeds it WiFi events (in the order they happened) and
// mesh-channel scan results, never WiFi.status(), and carries out every
// action it returns; a Drop must be safe to repeat. Contract: nothing scans
// or joins unless the network is wanted; a join begins only after a scan
// found the SSID; failures retry on a RetryBackoff; a link that appears while
// the network is unwanted or halted is dropped.

#pragma once

#include <cstdint>

#include "util/retry_backoff.hpp"

namespace wisp {

// The STA serves only the OSC feed, so it is wanted in Osc unless Ethernet
// already carries the feed.
constexpr bool staWanted(bool sourceIsOsc, bool ethernetUp) {
  return sourceIsOsc && !ethernetUp;
}

enum class StaAction : uint8_t {
  None,
  StartScan,  // scan the mesh channel for the stored SSID
  Begin,      // join the AP the scan found
  Drop,       // stop any scan or join, drop the association
};

class StaJoin {
 public:
  enum class State : uint8_t {
    Idle,
    Waiting,
    Scanning,
    Joining,
    Linked,
    Halted,
  };

  static constexpr uint32_t kScanTimeoutMs = 5000;
  static constexpr uint32_t kJoinTimeoutMs = 15000;

  State state() const { return state_; }

  // Whether the source uses the network. Wanting arms a scan at once, and
  // clears a halt; unwanting drops everything and retries nothing.
  StaAction setWanted(bool wanted, uint32_t nowMs) {
    if (wanted == wanted_) return StaAction::None;
    wanted_ = wanted;
    if (wanted) {
      arm(nowMs);
      return StaAction::None;
    }
    return idle();
  }

  // Creds changed or a reconnect was asked for: drop whatever is in flight
  // and, while wanted, start over at once. Clears a halt.
  StaAction restart(uint32_t nowMs) {
    if (!wanted_) return idle();
    const StaAction drop = state_ == State::Idle ? StaAction::None
                                                 : StaAction::Drop;
    arm(nowMs);
    return drop;
  }

  // A guard refused the link (wrong channel): drop and stay down until
  // restart() or the network is wanted again.
  StaAction halt() {
    const StaAction drop = state_ == State::Idle || state_ == State::Halted
                               ? StaAction::None
                               : StaAction::Drop;
    state_ = State::Halted;
    return drop;
  }

  // Drives scan starts and timeouts. Call every loop pass.
  StaAction step(uint32_t nowMs, bool hasCreds) {
    switch (state_) {
      case State::Waiting:
        if (!backoff_.due(nowMs)) return StaAction::None;
        if (!hasCreds) {
          state_ = State::Idle;
          return StaAction::None;
        }
        state_ = State::Scanning;
        startMs_ = nowMs;
        return StaAction::StartScan;
      case State::Scanning:
        if (nowMs - startMs_ < kScanTimeoutMs) return StaAction::None;
        fail(nowMs);
        return StaAction::Drop;
      case State::Joining:
        if (nowMs - startMs_ < kJoinTimeoutMs) return StaAction::None;
        fail(nowMs);
        return StaAction::Drop;
      default:
        return StaAction::None;
    }
  }

  // The mesh-channel scan finished. Found: join it. Not found: a failure,
  // and no wider scan.
  StaAction onScanDone(bool found, uint32_t nowMs) {
    if (state_ != State::Scanning) return StaAction::None;
    if (!found) {
      fail(nowMs);
      return StaAction::None;
    }
    state_ = State::Joining;
    startMs_ = nowMs;
    return StaAction::Begin;
  }

  // GOT_IP. Adopted in any wanted state but Halted, so a join the core
  // retried on its own still counts; while unwanted or halted it is dropped.
  StaAction onConnected() {
    if (!wanted_ || state_ == State::Halted) return StaAction::Drop;
    state_ = State::Linked;
    return StaAction::None;
  }

  // An STA disconnect the wisp did not ask for. Fails a join; a lost link
  // retries after the first backoff step.
  void onDisconnected(uint32_t nowMs) {
    if (state_ == State::Joining) {
      fail(nowMs);
    } else if (state_ == State::Linked) {
      backoff_.reset(nowMs);
      fail(nowMs);
    }
  }

  // The wait the last failure scheduled, for the log.
  uint32_t lastWaitMs() const { return lastWaitMs_; }

 private:
  void arm(uint32_t nowMs) {
    backoff_.reset(nowMs);
    state_ = State::Waiting;
  }

  // Drops from any state but Idle: from Waiting too, since the core may be
  // running a retry of its own that could still link.
  StaAction idle() {
    const StaAction drop = state_ == State::Idle ? StaAction::None
                                                 : StaAction::Drop;
    state_ = State::Idle;
    return drop;
  }

  void fail(uint32_t nowMs) {
    lastWaitMs_ = backoff_.failed(nowMs);
    state_ = State::Waiting;
  }

  State state_ = State::Idle;
  bool wanted_ = false;
  uint32_t startMs_ = 0;
  uint32_t lastWaitMs_ = 0;
  RetryBackoff backoff_;
};

}  // namespace wisp
