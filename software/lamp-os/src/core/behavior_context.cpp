#include "core/behavior_context.hpp"

#include <Arduino.h>

#include "components/network/mesh/lamp_roster.hpp"
#include "config/config.hpp"
#include "core/compositor.hpp"
#include "core/personality_engine.hpp"

extern lamp::Config config;

namespace lamp {

void BehaviorContext::forEachArrival(
    uint32_t maxAgeMs, const std::function<bool(const PeerView&)>& cb) {
  if (!lampRoster) return;
  RosterEntry arrival;
  if (!lampRoster->bestUngreetedArrival(
          maxAgeMs, millis(),
          [](const RosterEntry&) { return true; }, arrival)) {
    return;
  }
  if (ignoresPeers()) {
    if (discardsPeers()) lampRoster->acknowledge(arrival.mac);
    return;
  }
  PeerView v = PeerView::from(arrival);
  if (cb(v)) lampRoster->acknowledge(arrival.mac);
}

bool BehaviorContext::ignoresPeers() const {
  return compositor && compositor->blocksRemote(TransientSource::Remote);
}

bool BehaviorContext::discardsPeers() const {
  return compositor && compositor->discardsRemote(TransientSource::Remote);
}

void BehaviorContext::forEachNearby(
    const std::function<bool(const PeerView&)>& cb) {
  if (!lampRoster || ignoresPeers()) return;
  // Snapshot before invoking `cb`: a callback re-entering getMesh/getAll
  // fills a distinct buffer, so this snapshot stays valid across the walk.
  const std::vector<NearbyCopy>& near =
      lampRoster->snapshotNear(LAMP_PRUNE_TIME_MS);
  for (const NearbyCopy& e : near) {
    PeerView v = PeerView::make(e.name, e.mac, e.hasMac, e.baseColor,
                                e.shadeColor, e.lastRssi, e.variant);
    if (cb(v)) return;
  }
}

GreetingTuning BehaviorContext::greetingFor(const std::string& peerLampId) const {
  return personalityEngine.greetingFor(peerLampId);
}

uint8_t BehaviorContext::dispositionOf(const std::string& peerLampId) const {
  return ::config.getDisposition(peerLampId);
}

CrowdComposition BehaviorContext::crowd() const {
  if (ignoresPeers()) return {};
  return personalityEngine.crowdComposition();
}

float BehaviorContext::crowdWeight() const {
  if (ignoresPeers()) return 0.0f;
  return personalityEngine.smoothedCrowdWeight();
}

}  // namespace lamp
