#include "core/behavior_context.hpp"

#include "components/network/mesh/mesh_link.hpp"
#include "core/arrival_notifier.hpp"

namespace lamp {

void BehaviorContext::onArrival(std::function<void(const PeerView&)> cb) {
  arrivalNotifier.onArrival([this, cb = std::move(cb)](const PeerView& peer) {
    if (!ignoresPeers()) cb(peer);
  });
}

}  // namespace lamp
