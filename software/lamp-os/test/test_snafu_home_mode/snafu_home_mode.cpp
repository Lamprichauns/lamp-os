// Native pin for snafu at home with social off: the dots refuse a peer palette
// borrow and drift an active one back to the own palette, and the greeting
// takes no peer colour info. BehaviorContext::ignoresPeers is the seam; the
// rest is the shipped DotsBehavior and Greeting.

#include <unity.h>

#include "../../src/core/animated_behavior.cpp"
#include "../../src/core/frame_buffer.cpp"
#include "../../src/lamps/snafu/dots_behavior.cpp"
#include "../../src/lamps/snafu/greeting.cpp"
#include "../../src/util/color.cpp"
#include "../../src/util/fade.cpp"
#include "../../src/util/gradient.cpp"
#include "../../src/config/disposition_store.cpp"
#include "../../src/util/heap_guard.cpp"
#include "../../src/components/network/mesh/lamp_roster.cpp"
#include "../../src/core/peer_view.cpp"

static bool g_ignore = false;

lamp::Config config;
lamp::ExpressionManager expressionManager;

namespace lamp {
OverrideAggregate overrides;
PersonalityEngine personalityEngine;
GreetingTuning PersonalityEngine::greetingFor(const std::string&) const {
  GreetingTuning t;
  t.holdFrames = 100;
  return t;
}
bool Compositor::wispActive() const { return false; }
bool BehaviorContext::ignoresPeers() const { return g_ignore; }
void BehaviorContext::forEachArrival(uint32_t, const std::function<bool(const PeerView&)>&) {}
void ExpressionManager::sendInvocationTo(const uint8_t*, const ExpressionInvocation&) {}
void ExpressionManager::broadcastInvocation(const ExpressionInvocation&, const uint8_t*) {}
bool MeshLink::sendColorQuery(const uint8_t*) { return true; }
}  // namespace lamp

using namespace lamp;
using lamp::snafu::DotsBehavior;
using lamp::snafu::Greeting;

static Adafruit_NeoPixel neo0;

namespace {

const Color kOwn(0x40, 0, 0, 0);
const Color kPeer(0, 0x80, 0, 0);
const uint8_t kPeerMac[6] = {1, 2, 3, 4, 5, 6};

ShadeSettings ownConfig() {
  ShadeSettings cfg;
  cfg.segments = {{"a", 4, {kOwn}}};
  return cfg;
}

FrameBuffer makeFb() {
  FrameBuffer fb;
  fb.begin({}, std::vector<StripSegment>{{&neo0, "a", 0, 4}});
  return fb;
}

bool showsPeer(const FrameBuffer& fb) {
  for (const Color& c : fb.buffer) if (c.g > 0) return true;
  return false;
}

void drawFrames(DotsBehavior& d, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) d.draw();
}

}  // namespace

void setUp() { g_ignore = false; }
void tearDown() {}

void test_borrow_shows_peer_palette_outside_home_mode() {
  ShadeSettings cfg = ownConfig();
  FrameBuffer fb = makeFb();
  BehaviorContext ctx;
  DotsBehavior dots(&fb, cfg);
  dots.setBehaviorContext(&ctx);
  dots.borrowColors({kPeer}, {kPeer}, 1000);
  dots.draw();
  TEST_ASSERT_TRUE(showsPeer(fb));
}

void test_borrow_refused_while_ignoring_peers() {
  ShadeSettings cfg = ownConfig();
  FrameBuffer fb = makeFb();
  BehaviorContext ctx;
  DotsBehavior dots(&fb, cfg);
  dots.setBehaviorContext(&ctx);
  g_ignore = true;
  dots.borrowColors({kPeer}, {kPeer}, 1000);
  dots.draw();
  TEST_ASSERT_FALSE(showsPeer(fb));
}

// An active borrow drifts back to the own palette once home mode starts
// ignoring peers: still on show the first frame, gone after the crossfade.
void test_active_borrow_drifts_home_when_ignoring_starts() {
  ShadeSettings cfg = ownConfig();
  FrameBuffer fb = makeFb();
  BehaviorContext ctx;
  DotsBehavior dots(&fb, cfg);
  dots.setBehaviorContext(&ctx);
  dots.borrowColors({kPeer}, {kPeer}, 100000);
  dots.draw();

  g_ignore = true;
  dots.control();
  dots.draw();
  TEST_ASSERT_TRUE(showsPeer(fb));
  drawFrames(dots, DotsBehavior::kBorrowSceneFrames);
  TEST_ASSERT_FALSE(showsPeer(fb));

  // Home mode ending does not bring the borrow back.
  g_ignore = false;
  dots.control();
  dots.draw();
  TEST_ASSERT_FALSE(showsPeer(fb));
}

// The greeting's colour info feeds the post-greeting borrow; while ignoring
// peers it takes none, so nothing is borrowed even once home mode ends.
static bool greetingBorrowsAfterColourInfo(bool ignoreDuringInfo) {
  ShadeSettings cfg = ownConfig();
  FrameBuffer shadeFb = makeFb();
  FrameBuffer dotsFb = makeFb();
  LampRoster roster;
  BehaviorContext ctx;
  ctx.lampRoster = &roster;
  DotsBehavior dots(&dotsFb, cfg);
  dots.setBehaviorContext(&ctx);
  Greeting greeting(&shadeFb);
  greeting.setBehaviorContext(&ctx);
  greeting.setDotsBehavior(&dots);

  PeerView peer;
  peer.mac = kPeerMac;
  peer.hasMac = true;
  peer.baseColor = kPeer;
  greeting.triggerGreeting(peer);

  g_ignore = ignoreDuringInfo;
  greeting.onColorInfo(kPeerMac, {kPeer}, {kPeer});
  g_ignore = false;

  greeting.animationState = STOPPED;  // the greeting's pass ends
  greeting.control();
  dots.draw();
  return showsPeer(dotsFb);
}

void test_greeting_takes_colour_info_outside_home_mode() {
  TEST_ASSERT_TRUE(greetingBorrowsAfterColourInfo(false));
}

void test_greeting_takes_no_colour_info_while_ignoring_peers() {
  TEST_ASSERT_FALSE(greetingBorrowsAfterColourInfo(true));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_borrow_shows_peer_palette_outside_home_mode);
  RUN_TEST(test_borrow_refused_while_ignoring_peers);
  RUN_TEST(test_active_borrow_drifts_home_when_ignoring_starts);
  RUN_TEST(test_greeting_takes_colour_info_outside_home_mode);
  RUN_TEST(test_greeting_takes_no_colour_info_while_ignoring_peers);
  return UNITY_END();
}
