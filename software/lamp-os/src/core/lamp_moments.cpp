// core/lamp_moments.cpp: binds the Moment director to the live lamp.
//
// Sibling TU of lamp.cpp; shares file-scope state via core/lamp_internal.hpp.

#include "core/lamp.hpp"
#include "core/lamp_internal.hpp"

#include <Arduino.h>

#include "behaviors/greetable.hpp"
#include "components/firmware/ota_quiet_mode.hpp"
#include "core/compositor.hpp"
#include "core/frame_buffer.hpp"
#include "core/moments.hpp"
#include "core/personality_engine.hpp"
#include "esp_system.h"

namespace {

// A wisp presence above this means the wisp holds the surface. Pulse ignores
// the wisp-dim and a flicker burst still shows at its 0.3 floor, so either
// would draw over the show.
constexpr float kWispHolds = 0.01f;

class LampMomentHost : public lamp::MomentHost {
 public:
  const std::vector<lamp::NearbyCopy>& nearbyPeers() override {
    return lamp::personalityEngine.nearbyPeers();
  }
  uint8_t disposition(const uint8_t mac[6]) override { return config.getDisposition(mac); }
  lamp::SocialMode socialMode() override { return config.lamp.socialMode; }
  const char* suppressor() override {
    if (meshLink.isOtaInProgress() || lamp::ota_quiet_mode::isQuiet()) return "ota";
    if (compositor.wispPresence(false) > kWispHolds || compositor.wispPresence(true) > kWispHolds) {
      return "wisp";
    }
    if (expressionManager.isAnyTestActive()) return "preview";
    if (compositor.homeSuppressesSocial()) return "home";
    return nullptr;
  }
  bool greetingActive() override {
    lamp::BehaviorContext& ctx = compositor.behaviorContext();
    return ctx.greeting && ctx.greeting->greetingState().active;
  }
  bool canSend() override { return meshLink.txReady() && !meshLink.isOtaInProgress(); }
  void send(const char* payload, size_t len) override {
    meshLink.sendEvent(reinterpret_cast<const uint8_t*>(payload), len);
  }
  bool trigger(const lamp::ExpressionInvocation& inv, const uint8_t srcMac[6], bool broadcast,
               lamp::TransientSource source) override {
    return expressionManager.triggerInvocation(inv, srcMac, broadcast, source);
  }
  uint16_t shadePixels() override { return shade.pixelCount; }
  const std::vector<lamp::Color>& surfaceColors(bool isBase) override {
    return isBase ? config.base.broadcastColors() : config.shade.broadcastColors();
  }
  void log(const char* line) override { Serial.println(line); }
};

LampMomentHost s_momentHost;

}  // namespace

void beginMoments(const uint8_t ownMac[6]) {
  lamp::momentDirector.begin(&s_momentHost, ownMac,
                             lamp::momentHashMac(ownMac) ^ esp_random(), millis());
}
