// Native pin for the arrival side of home mode's social-off: while home mode
// is active with social off, BehaviorContext hands no arrival to a greeting
// (forEachArrival, SocialBehavior's own roster scan) or an arrival reaction
// (onArrival), reads the room as empty (forEachNearby), and consumes arrivals
// so leaving home does not greet a room that was there all along; the app's
// Home Mode preview consumes nothing. Runs the shipped BehaviorContext,
// ArrivalNotifier and LampRoster.

#include <unity.h>

#include "core/animated_behavior.cpp"
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
// BehaviorContext's social reads link against these; the crowd stubs report a
// non-empty room. The compositor is used for its inline home-mode gate; the policy
// setter stores what compositor.cpp's does, without its render dependencies.
PersonalityEngine personalityEngine;
GreetingTuning PersonalityEngine::greetingFor(const std::string&) const { return {}; }
CrowdComposition PersonalityEngine::crowdComposition() const {
  CrowdComposition c;
  c.fond = 2;
  return c;
}
float PersonalityEngine::smoothedCrowdWeight() const { return 1.5f; }
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

// The crowd reads report an empty room while home mode ignores peers.
void test_crowd_reads_empty_in_home_social_off() {
  Rig r;
  TEST_ASSERT_EQUAL_UINT8(2, r.ctx.crowd().fond);
  TEST_ASSERT_EQUAL_FLOAT(1.5f, r.ctx.crowdWeight());

  r.compositor.homeMode = true;
  TEST_ASSERT_EQUAL_UINT8(0, r.ctx.crowd().fond);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, r.ctx.crowdWeight());
}

// A peer that leaves and returns within a preview fires once the preview ends,
// as it would have without one.
void test_preview_rearms_a_peer_that_left_and_returned() {
  Rig r;
  arrivalNotifier = ArrivalNotifier();
  int fires = 0;
  r.ctx.onArrival([&](const PeerView&) { ++fires; });
  const uint32_t nearWindow = 20000;

  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  r.ctx.tickArrivals(100010, nearWindow);
  TEST_ASSERT_EQUAL_INT(1, fires);

  r.compositor.homeMode = true;
  r.compositor.homePreview = true;
  const uint32_t departed = 100000 + nearWindow + 5000;
  set_mock_millis(departed);
  r.ctx.tickArrivals(departed, nearWindow);
  const uint32_t back = departed + 2000;
  set_mock_millis(back);
  r.seedNear("jacko", "AA:BB:CC:DD:EE:F0");
  r.ctx.tickArrivals(back, nearWindow);
  TEST_ASSERT_EQUAL_INT(1, fires);

  r.compositor.homeMode = false;
  r.compositor.homePreview = false;
  const uint32_t after = back + 1000;
  set_mock_millis(after);
  r.ctx.tickArrivals(after, nearWindow);
  TEST_ASSERT_EQUAL_INT(2, fires);
}

// The compositor's per-behavior decision for a social behavior: cancelled
// while home mode ignores peers, run while the app's own greeting plays.
namespace {
struct FakeSocial : AnimatedBehavior {
  bool appGreeting = false;
  bool isSocialBehavior() const override { return true; }
  bool playsAppGreeting() const override { return appGreeting; }
};
}  // namespace

void test_compositor_cancels_social_unless_app_greeting() {
  Compositor compositor;
  FakeSocial social;
  TEST_ASSERT_TRUE(compositor.homeModeTickFor(&social) == HomeModeTick::Run);
  compositor.homeMode = true;
  TEST_ASSERT_TRUE(compositor.homeModeTickFor(&social) == HomeModeTick::Cancel);
  social.appGreeting = true;
  TEST_ASSERT_TRUE(compositor.homeModeTickFor(&social) == HomeModeTick::Run);
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
  RUN_TEST(test_crowd_reads_empty_in_home_social_off);
  RUN_TEST(test_preview_rearms_a_peer_that_left_and_returned);
  RUN_TEST(test_compositor_cancels_social_unless_app_greeting);
  return UNITY_END();
}
