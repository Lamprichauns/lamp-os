// Native pin for the arrival side of home mode's social-off: while home mode
// is active with social off, BehaviorContext hands no arrival to a greeting
// (forEachArrival) or an arrival reaction (onArrival), and consumes it so
// leaving home does not greet a room that was there all along. Runs the
// shipped BehaviorContext, ArrivalNotifier and LampRoster.

#include <unity.h>

#include "core/arrival_notifier.cpp"
#include "core/behavior_context.cpp"
#include "core/behavior_context_mesh.cpp"
#include "components/network/mesh/lamp_roster.cpp"
#include "core/peer_view.cpp"
#include "config/disposition_store.cpp"
#include "util/heap_guard.cpp"

lamp::ArrivalNotifier arrivalNotifier;
lamp::Config config;

namespace lamp {
// BehaviorContext's social reads link against these; the arrival tests never
// call them. The compositor is used for its inline home-mode gate; the policy
// setter stores what compositor.cpp's does, without its render dependencies.
PersonalityEngine personalityEngine;
GreetingTuning PersonalityEngine::greetingFor(const std::string&) const { return {}; }
CrowdComposition PersonalityEngine::crowdComposition() const { return {}; }
float PersonalityEngine::smoothedCrowdWeight() const { return 0.0f; }
uint8_t Config::getDisposition(const std::string&) const { return 3; }
Compositor::Compositor() {}
void Compositor::setHomeModePolicy(bool socialDisabled, std::vector<std::string> disabledIds) {
  homeSocialDisabled_ = socialDisabled;
  homeDisabledExprIds_ = std::move(disabledIds);
}
}  // namespace lamp

using namespace lamp;

namespace {

const Color kNoColor = Color();
constexpr uint32_t kNearWindowMs = 240000;

struct Rig {
  LampRoster roster;
  Compositor compositor;
  BehaviorContext ctx;
  Rig() {
    ctx.lampRoster = &roster;
    ctx.compositor = &compositor;
  }
  void seedNear(const char* name, const char* bleAddr) {
    roster.addOrUpdateFromBle(name, bleAddr, kNoColor, kNoColor, -50);
  }
  int pull() {
    int greeted = 0;
    ctx.forEachArrival(5000, [&](const PeerView&) {
      ++greeted;
      return true;
    });
    return greeted;
  }
};

}  // namespace

void setUp() { set_mock_millis(100000); }
void tearDown() {}

// Home mode on, social off (the default policy): a greeting sees no arrival.
void test_pull_arrival_ignored_in_home_social_off() {
  Rig r;
  r.compositor.homeMode = true;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  TEST_ASSERT_EQUAL_INT(0, r.pull());

  // Consumed: home mode ending does not hand the same arrival over late.
  r.compositor.homeMode = false;
  TEST_ASSERT_EQUAL_INT(0, r.pull());
}

void test_pull_arrival_greeted_outside_home_mode() {
  Rig r;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  TEST_ASSERT_EQUAL_INT(1, r.pull());
}

void test_pull_arrival_greeted_in_home_with_social_on() {
  Rig r;
  r.compositor.homeMode = true;
  r.compositor.setHomeModePolicy(/*socialDisabled=*/false, {"glitchy"});
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  TEST_ASSERT_EQUAL_INT(1, r.pull());
}

void test_push_arrival_ignored_in_home_social_off() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  int fires = 0;
  r.ctx.onArrival([&](const PeerView&) { ++fires; });

  r.compositor.homeMode = true;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  arrivalNotifier.tick(100010, r.roster, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(0, fires);

  // Consumed: still near after home mode ends, no late fire.
  r.compositor.homeMode = false;
  set_mock_millis(102000);
  arrivalNotifier.tick(102000, r.roster, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(0, fires);
}

void test_push_arrival_fires_outside_home_mode() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  int fires = 0;
  r.ctx.onArrival([&](const PeerView&) { ++fires; });

  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  arrivalNotifier.tick(100010, r.roster, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(1, fires);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pull_arrival_ignored_in_home_social_off);
  RUN_TEST(test_pull_arrival_greeted_outside_home_mode);
  RUN_TEST(test_pull_arrival_greeted_in_home_with_social_on);
  RUN_TEST(test_push_arrival_ignored_in_home_social_off);
  RUN_TEST(test_push_arrival_fires_outside_home_mode);
  return UNITY_END();
}
