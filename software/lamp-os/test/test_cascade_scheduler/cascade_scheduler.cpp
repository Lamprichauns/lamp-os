// Native tests for the cascade fan-out drain: every peer is reached, at most a
// batch is queued per pass, each frame's delayMs keeps the peer's intended
// fire time, and the frames are byte-identical to serializeInvocation.

#include <unity.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "expressions/cascade_scheduler.hpp"
#include "expressions/expression_invocation.hpp"

#include "../../src/util/color.cpp"
#include "../../src/expressions/expression_invocation.cpp"
#include "../../src/expressions/cascade_scheduler.cpp"

using namespace lamp;

void setUp(void) {}
void tearDown(void) {}

namespace {

struct Sent {
  uint8_t peer;
  std::string payload;
};

ExpressionInvocation sampleInvocation() {
  ExpressionInvocation inv;
  inv.type = "glitchy";
  inv.target = 3;
  inv.colors = {Color(1, 2, 3, 4)};
  inv.parameters = {{"durationMin", 250}, {"durationMax", 900}};
  return inv;
}

void beginWith(CascadeScheduler& s, const ExpressionInvocation& inv, size_t peers,
               uint32_t staggerMs, uint32_t nowMs) {
  std::string json;
  serializeInvocation(inv, json);
  TEST_ASSERT_TRUE(s.begin(json.data(), json.size(), staggerMs, nowMs));
  for (size_t i = 0; i < peers; ++i) {
    const uint8_t mac[6] = {0x02, 0, 0, 0, 0, static_cast<uint8_t>(i)};
    s.addPeer(mac);
  }
}

size_t drain(CascadeScheduler& s, uint32_t nowMs, std::vector<Sent>& out) {
  return s.tick(nowMs, [&](const uint8_t* mac, const char* payload, size_t len) {
    out.push_back({mac[5], std::string(payload, len)});
  });
}

}  // namespace

void test_delay_keeps_the_intended_fire_time() {
  TEST_ASSERT_EQUAL_UINT32(100, CascadeScheduler::delayFor(0, 100, 0));
  TEST_ASSERT_EQUAL_UINT32(500, CascadeScheduler::delayFor(4, 100, 0));
  TEST_ASSERT_EQUAL_UINT32(450, CascadeScheduler::delayFor(4, 100, 50));
  TEST_ASSERT_EQUAL_UINT32(kMaxDelayMs, CascadeScheduler::delayFor(20, 100, 0));
  TEST_ASSERT_EQUAL_UINT32(kMaxDelayMs - 50, CascadeScheduler::delayFor(20, 100, 50));
  TEST_ASSERT_EQUAL_UINT32(0, CascadeScheduler::delayFor(0, 100, 150));
  TEST_ASSERT_EQUAL_UINT32(0, CascadeScheduler::delayFor(3, 0, 10));
  TEST_ASSERT_EQUAL_UINT32(kMaxDelayMs, CascadeScheduler::delayFor(49, 0xFFFFFFFFu, 0));
}

// 50 peers drain in batches of 8, one batch per gap, every peer once, closest
// first, each frame what serializeInvocation writes for its remaining delay.
void test_every_peer_is_reached_in_bounded_batches() {
  CascadeScheduler s;
  const ExpressionInvocation inv = sampleInvocation();
  const uint32_t start = 1000;
  const uint32_t stagger = 40;
  beginWith(s, inv, 50, stagger, start);
  std::vector<Sent> sent;
  uint32_t now = start;
  size_t batches = 0;
  while (s.active()) {
    const size_t n = drain(s, now, sent);
    TEST_ASSERT_TRUE(n <= CascadeScheduler::kBatchSize);
    if (n > 0) ++batches;
    TEST_ASSERT_EQUAL(0, drain(s, now + CascadeScheduler::kBatchGapMs - 1, sent));
    now += CascadeScheduler::kBatchGapMs;
  }
  TEST_ASSERT_EQUAL(50, sent.size());
  TEST_ASSERT_EQUAL(7, batches);
  for (size_t i = 0; i < sent.size(); ++i) {
    TEST_ASSERT_EQUAL_UINT8(i, sent[i].peer);
    const uint32_t elapsed = static_cast<uint32_t>(i / CascadeScheduler::kBatchSize) *
                             CascadeScheduler::kBatchGapMs;
    ExpressionInvocation expected = inv;
    expected.delayMs = CascadeScheduler::delayFor(i, stagger, elapsed);
    std::string json;
    serializeInvocation(expected, json);
    TEST_ASSERT_EQUAL_STRING(json.c_str(), sent[i].payload.c_str());
  }
}

void test_a_new_cascade_replaces_the_remainder() {
  CascadeScheduler s;
  beginWith(s, sampleInvocation(), 20, 0, 0);
  std::vector<Sent> sent;
  drain(s, 0, sent);
  TEST_ASSERT_EQUAL(12, s.pending());
  beginWith(s, sampleInvocation(), 3, 0, 10);
  TEST_ASSERT_EQUAL(3, s.pending());
  drain(s, 10, sent);
  TEST_ASSERT_FALSE(s.active());
}

void test_cancel_stops_the_drain() {
  CascadeScheduler s;
  beginWith(s, sampleInvocation(), 20, 0, 0);
  s.cancel();
  std::vector<Sent> sent;
  TEST_ASSERT_EQUAL(0, drain(s, 0, sent));
}

void test_oversize_payload_is_refused() {
  CascadeScheduler s;
  std::string json = "{\"delayMs\":0,\"pad\":\"" +
                     std::string(CascadeScheduler::kMaxPayloadLen, 'x') + "\"}";
  TEST_ASSERT_FALSE(s.begin(json.data(), json.size(), 0, 0));
  TEST_ASSERT_FALSE(s.active());
}

// The payload buffer holds the longest invocation the caps allow, with the
// widest delay.
void test_worst_case_invocation_fits() {
  ExpressionInvocation inv;
  inv.type = std::string(kMaxExpressionTypeLen, 't');
  inv.target = 3;
  inv.delayMs = kMaxDelayMs;
  inv.colors.assign(kMaxExpressionColors, Color(255, 255, 255, 255));
  std::vector<size_t> byLength;
  for (size_t i = 0; i < kExpressionParamKeyCount; ++i) byLength.push_back(i);
  std::sort(byLength.begin(), byLength.end(), [](size_t a, size_t b) {
    return std::strlen(kExpressionParamKeys[a]) > std::strlen(kExpressionParamKeys[b]);
  });
  for (size_t i = 0; i < kMaxExpressionParams; ++i) {
    inv.parameters.setIndex(byLength[i], 0xFFFFFFFFu);
  }
  std::string json;
  serializeInvocation(inv, json);
  printf("[cascade] worst-case invocation %zu of %zu bytes\n", json.size(),
         CascadeScheduler::kMaxPayloadLen);
  TEST_ASSERT_TRUE(json.size() <= CascadeScheduler::kMaxPayloadLen);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_delay_keeps_the_intended_fire_time);
  RUN_TEST(test_every_peer_is_reached_in_bounded_batches);
  RUN_TEST(test_a_new_cascade_replaces_the_remainder);
  RUN_TEST(test_cancel_stops_the_drain);
  RUN_TEST(test_oversize_payload_is_refused);
  RUN_TEST(test_worst_case_invocation_fits);
  return UNITY_END();
}
