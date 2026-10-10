// Native pin for SocialBehavior::cancelGreeting, which the compositor calls on
// a SocialBehavior home mode skips: a greeting in progress ends (STOPPED,
// frame 0) and the app is told once; with none playing it does nothing.
// Links the shipped social.cpp; its OTA, roster and render dependencies are
// link stubs this test never reaches.

#include <unity.h>

#include "../../src/core/animated_behavior.cpp"
#include "../../src/behaviors/social.cpp"
#include "../../src/components/network/mesh/lamp_roster.cpp"
#include "../../src/core/peer_view.cpp"
#include "../../src/util/color.cpp"
#include "../../src/util/fade.cpp"
#include "../../src/util/gradient.cpp"

lamp::FirmwareReceiver firmwareReceiver;

namespace lamp {
OverrideAggregate overrides;
PersonalityEngine personalityEngine;
FirmwareDistributor firmwareDistributor;
GreetingTuning PersonalityEngine::greetingFor(const uint8_t*) const { return {}; }
bool FirmwareDistributor::isInProgress() const { return false; }
void FirmwareDistributor::considerPeerForOta(const uint8_t*, uint32_t, uint8_t, uint32_t,
                                             const char*, uint16_t, int8_t, bool) {}
bool MeshLink::sendColorQuery(const uint8_t*) { return true; }
}  // namespace lamp

namespace fs_ota {
bool fsPathBusy() { return false; }
void considerPeer(const uint8_t*, uint32_t, uint8_t, uint32_t, const char*, const uint8_t*,
                  bool, bool, int8_t) {}
}  // namespace fs_ota

using namespace lamp;

namespace {
const uint8_t kPeerMac[6] = {1, 2, 3, 4, 5, 6};

PeerView peer() {
  PeerView p;
  p.mac = kPeerMac;
  p.hasMac = true;
  return p;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_cancel_ends_greeting_and_tells_app_once() {
  SocialBehavior social(nullptr, 1200);
  int notifies = 0;
  social.setOnGreetingChangeCallback([&]() { ++notifies; });
  social.triggerGreeting(peer());
  social.frame = 40;
  TEST_ASSERT_TRUE(social.greetingState().active);
  notifies = 0;

  social.cancelGreeting();
  TEST_ASSERT_EQUAL_INT(STOPPED, social.animationState);
  TEST_ASSERT_EQUAL_UINT32(0, social.frame);
  TEST_ASSERT_FALSE(social.greetingState().active);
  TEST_ASSERT_EQUAL_INT(1, notifies);

  // The compositor calls it every skipped tick, and control() resumes after
  // home mode: neither tells the app again or restarts the greeting.
  social.cancelGreeting();
  social.control();
  TEST_ASSERT_EQUAL_INT(1, notifies);
  TEST_ASSERT_EQUAL_INT(STOPPED, social.animationState);
}

void test_cancel_without_greeting_is_noop() {
  SocialBehavior social(nullptr, 1200);
  int notifies = 0;
  social.setOnGreetingChangeCallback([&]() { ++notifies; });
  social.cancelGreeting();
  TEST_ASSERT_EQUAL_INT(0, notifies);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_cancel_ends_greeting_and_tells_app_once);
  RUN_TEST(test_cancel_without_greeting_is_noop);
  return UNITY_END();
}
