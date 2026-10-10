#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "core/behavior_context.hpp"  // PeerView

namespace lamp {

class LampRoster;

// What one ArrivalNotifier::tick does with a new near peer.
enum class ArrivalPass : uint8_t {
  Fire,     // a fresh arrival fires the observers
  Silence,  // every near peer is marked fired without firing, fresh or not
  Hold,     // nothing is marked or fired; departures still re-arm
};

// Push-notifies registered observers once per genuinely-new near peer.
//
// Dedup is framework-owned and does NOT touch the greeting `acknowledged`
// flag: a fixed MAC set records who has fired. A peer re-fires only after it
// leaves the near window (departure, an edge on lastSeenNearMs vs the near
// window) and later returns; roster prune/lifetime is irrelevant, a
// mesh-reachable peer that walks out of BLE range never prunes. The diff runs
// every tick over `snapshotNear`, whose freshness cache
// (LampRoster::kSnapshotCacheMs) keeps the roster snapshot off the hot path
// (docs/dev/embedded-heap.md). Per-peer state is raw MAC bytes; the callbacks
// are a small attach-once set, not per-peer.
class ArrivalNotifier {
 public:
  using Callback = std::function<void(const PeerView&)>;

  static constexpr size_t kMaxObservers = 4;
  // Equal to LampRoster::kCapacity, so every near peer has a slot and a
  // silenced pass marks the whole room.
  static constexpr size_t kMaxFired = 50;
  // A sighting older than this is "been around", not a fresh arrival.
  static constexpr uint32_t kArrivalMaxAgeMs = 5000;

  // Attach-once at boot. Bounded; an excess registration is dropped.
  void onArrival(Callback cb);

  // Near-set diff. `nearWindowMs` is the departure hysteresis: a fired peer
  // re-arms once its last near sighting falls outside it, distinct from the
  // short kArrivalMaxAgeMs that bounds a fresh arrival. No-op when no observer
  // is registered. `pass` decides what a new near peer gets; see ArrivalPass.
  void tick(uint32_t nowMs, LampRoster& roster, uint32_t nearWindowMs,
            ArrivalPass pass = ArrivalPass::Fire);

  size_t observerCount() const { return observerCount_; }
  size_t firedCount() const;

 private:
  int firedIndex(const uint8_t mac[6]) const;
  void fire(const PeerView& v);

  Callback observers_[kMaxObservers];
  size_t observerCount_ = 0;
  uint8_t fired_[kMaxFired][6] = {};
  bool firedUsed_[kMaxFired] = {};
};

}  // namespace lamp

// Single global instance, defined in core/lamp.cpp (mirrors
// expressionObserverRegistry).
extern lamp::ArrivalNotifier arrivalNotifier;
