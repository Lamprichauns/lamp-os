#include "core/behavior_context.hpp"

#include "components/network/mesh/mesh_link.hpp"
#include "components/network/mesh/lamp_roster.hpp"
#include "core/arrival_notifier.hpp"

namespace lamp {

void BehaviorContext::onArrival(std::function<void(const PeerView&)> cb) {
  arrivalNotifier.onArrival(std::move(cb));
}

void BehaviorContext::tickArrivals(uint32_t nowMs, uint32_t nearWindowMs) {
  if (!lampRoster) return;
  if (discardsPeers()) {
    RosterEntry arrival;
    for (size_t i = 0; i < LampRoster::kCapacity &&
                       lampRoster->bestUngreetedArrival(
                           nearWindowMs, nowMs, [](const RosterEntry&) { return true; },
                           arrival);
         ++i) {
      lampRoster->acknowledge(arrival.mac);
    }
    arrivalNotifier.tick(nowMs, *lampRoster, nearWindowMs, /*silenced=*/true);
  } else if (!ignoresPeers()) {
    arrivalNotifier.tick(nowMs, *lampRoster, nearWindowMs);
  }
}

}  // namespace lamp
