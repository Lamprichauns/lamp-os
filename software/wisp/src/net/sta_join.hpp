// When the wisp's WiFi STA scans for, joins and drops its stored network.
// Pure: WifiLink feeds it WiFi events and scan results and carries out the
// actions it returns, so the policy is host-testable. Every attempt is judged
// by the events it produces, never by WiFi.status(), which the Arduino core
// latches from the last event and does not clear when a new join starts.
//
// One attempt: scan the mesh channel only for the SSID; begin a join only
// when it is there; a disconnect or a join that neither links nor fails
// within kJoinTimeoutMs counts as a failure. Failures retry on a
// RetryBackoff. Nothing runs while the source does not want the network.

#pragma once

#include <cstdint>

#include "util/retry_backoff.hpp"

namespace wisp {

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

  // Whether the source uses the network. Wanting arms a scan at once;
  // unwanting drops everything and retries nothing.
  StaAction setWanted(bool wanted, uint32_t nowMs) {
    if (wanted == wanted_) return StaAction::None;
    wanted_ = wanted;
    if (wanted) {
      arm(nowMs);
      return StaAction::None;
    }
    return idle();
  }

  // Creds changed or a reconnect was asked for: start over at once.
  StaAction restart(uint32_t nowMs) {
    const bool active = state_ == State::Scanning || state_ == State::Joining ||
                        state_ == State::Linked;
    if (!wanted_) {
      state_ = State::Idle;
      backoff_.stop();
      return active ? StaAction::Drop : StaAction::None;
    }
    arm(nowMs);
    return active ? StaAction::Drop : StaAction::None;
  }

  // A guard refused the link (wrong channel): drop and stay down until
  // restart().
  StaAction halt() {
    const bool active = state_ != State::Idle && state_ != State::Waiting &&
                        state_ != State::Halted;
    state_ = State::Halted;
    backoff_.stop();
    return active ? StaAction::Drop : StaAction::None;
  }

  // Drives scan starts and timeouts. Call every loop pass.
  StaAction step(uint32_t nowMs, bool hasCreds) {
    switch (state_) {
      case State::Waiting:
        if (!backoff_.due(nowMs)) return StaAction::None;
        if (!hasCreds) {
          state_ = State::Idle;
          backoff_.stop();
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

  // GOT_IP. Adopted in any wanted state, so a join the core retried on its
  // own still counts.
  void onConnected() {
    if (!wanted_ || state_ == State::Halted) return;
    state_ = State::Linked;
    backoff_.stop();
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

  StaAction idle() {
    const bool active = state_ == State::Scanning || state_ == State::Joining ||
                        state_ == State::Linked;
    state_ = State::Idle;
    backoff_.stop();
    return active ? StaAction::Drop : StaAction::None;
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
