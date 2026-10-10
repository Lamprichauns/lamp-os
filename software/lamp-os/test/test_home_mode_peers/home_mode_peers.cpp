// Native pin for the arrival side of home mode's social-off: while home mode
// is active with social off, BehaviorContext hands no arrival to a greeting
// (forEachArrival, SocialBehavior's own roster scan) or an arrival reaction
// (onArrival), reads the room as empty (forEachNearby), and consumes arrivals
// so leaving home does not greet a room that was there all along; the app's
// Home Mode preview consumes nothing. Runs the shipped BehaviorContext,
// ArrivalNotifier and LampRoster.

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

// Staff path: the onArrival notifier fires nothing at home, and a peer near
// throughout does not fire once home mode ends.
void test_push_arrival_ignored_in_home_social_off() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  int fires = 0;
  r.ctx.onArrival([&](const PeerView&) { ++fires; });

  r.compositor.homeMode = true;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  // Past the fresh-arrival window: marked anyway, since it is still near.
  set_mock_millis(106000);
  r.ctx.tickArrivals(106000, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(0, fires);

  r.compositor.homeMode = false;
  set_mock_millis(108000);
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");  // still near, freshly sighted
  r.ctx.tickArrivals(108000, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(0, fires);
}

void test_push_arrival_fires_outside_home_mode() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  int fires = 0;
  r.ctx.onArrival([&](const PeerView&) { ++fires; });

  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  r.ctx.tickArrivals(100010, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(1, fires);
}

// Standard path: SocialBehavior is skipped at home and scans the roster itself
// (bestUngreetedArrival over the prune window). The framework pass consumes
// every arrival, so leaving home greets nobody who was already near.
void test_framework_pass_consumes_arrivals_for_social_behavior() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  r.compositor.homeMode = true;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  r.seedNear("bob", "AA:BB:CC:DD:EE:B0");
  r.ctx.tickArrivals(100010, kNearWindowMs);

  r.compositor.homeMode = false;
  RosterEntry e;
  TEST_ASSERT_FALSE(r.roster.bestUngreetedArrival(
      kNearWindowMs, 100020, [](const RosterEntry&) { return true; }, e));
}

// The app's Home Mode preview forces home mode on but consumes nothing: once
// it ends, the arrival is greeted and the notifier fires.
void test_preview_holds_arrivals_without_consuming() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  int fires = 0;
  r.ctx.onArrival([&](const PeerView&) { ++fires; });
  r.compositor.homeMode = true;
  r.compositor.homePreview = true;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");

  r.ctx.tickArrivals(100010, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(0, r.pull());
  TEST_ASSERT_EQUAL_INT(0, fires);

  r.compositor.homeMode = false;
  r.compositor.homePreview = false;
  r.ctx.tickArrivals(100020, kNearWindowMs);
  TEST_ASSERT_EQUAL_INT(1, fires);
  TEST_ASSERT_EQUAL_INT(1, r.pull());
}

// Lions ambient and Loaf ring read the room through forEachNearby: it is
// empty while home mode ignores peers.
void test_nearby_reads_empty_in_home_social_off() {
  Rig r;
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  set_mock_millis(100000 + LampRoster::kSnapshotCacheMs);
  int seen = 0;
  auto count = [&](const PeerView&) { ++seen; return false; };

  r.compositor.homeMode = true;
  r.ctx.forEachNearby(count);
  TEST_ASSERT_EQUAL_INT(0, seen);

  r.compositor.homeMode = false;
  r.ctx.forEachNearby(count);
  TEST_ASSERT_EQUAL_INT(1, seen);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pull_arrival_ignored_in_home_social_off);
  RUN_TEST(test_pull_arrival_greeted_outside_home_mode);
  RUN_TEST(test_pull_arrival_greeted_in_home_with_social_on);
  RUN_TEST(test_push_arrival_ignored_in_home_social_off);
  RUN_TEST(test_push_arrival_fires_outside_home_mode);
  RUN_TEST(test_framework_pass_consumes_arrivals_for_social_behavior);
  RUN_TEST(test_preview_holds_arrivals_without_consuming);
  RUN_TEST(test_nearby_reads_empty_in_home_social_off);
  return UNITY_END();
}
