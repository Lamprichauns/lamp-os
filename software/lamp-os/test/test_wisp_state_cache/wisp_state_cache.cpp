// LampRoster::cacheWispState, the drain's feed into the claims blob: a STATE
// slot's W plane and painted entries (black included) reach CHAR_WISP_CLAIMS.

#include <unity.h>

#include <cstdint>
#include <cstring>

#include "components/network/mesh/lamp_roster.cpp"
#include "components/network/mesh/pending_slots.cpp"
#include "components/network/mesh/wisp_cache.cpp"
#include "components/network/mesh/wisp_fleet_cache.cpp"
#include "components/network/mesh/wisp_status_merge.cpp"

namespace lp = lamp_protocol;
using lamp::LampRoster;
using lamp::PendingWispState;
using lamp::WispFleetCache;

void setUp(void) {}
void tearDown(void) {}

namespace {
const uint8_t kSrc[6]   = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5};
const uint8_t kWarm[6]  = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
const uint8_t kBlack[6] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25};

PendingWispState gSlot;

void fill(bool withW) {
  uint8_t entries[2 * lp::WISP_STATE_ENTRY_SIZE] = {};
  std::memcpy(entries, kWarm, 6);
  std::memcpy(entries + 12, kBlack, 6);
  const uint8_t w[4] = {0xC8, 0x37, 0, 0};
  uint8_t buf[lp::WISP_STATE_MAX_SIZE];
  const size_t n = lp::buildWispState(buf, sizeof(buf), 1, kSrc, 100, 60000, 0,
                                      entries, withW ? w : nullptr, 2);
  lp::ParsedWispState ws;
  TEST_ASSERT_TRUE(lp::parseWispState(buf, n, ws));
  lamp::fillPendingWispState(gSlot, ws);
}
}  // namespace

void test_state_slot_reaches_claims_blob() {
  LampRoster roster;
  fill(true);
  roster.cacheWispState(gSlot, 1000);

  constexpr size_t kE = WispFleetCache::kBlobEntrySize;
  uint8_t blob[1 + 2 * kE];
  TEST_ASSERT_EQUAL_size_t(sizeof(blob),
                           roster.buildWispClaimsBlob(blob, sizeof(blob), 1000));
  TEST_ASSERT_EQUAL_UINT8(2, blob[0]);
  const size_t ext = 1 + 2 * 12;
  for (size_t i = 0; i < 2; ++i) {
    const uint8_t* mac = blob + 1 + i * 6;
    const uint8_t* e = blob + ext + i * 3;
    TEST_ASSERT_EQUAL_UINT8(1, e[0]);
    if (std::memcmp(mac, kWarm, 6) == 0) {
      TEST_ASSERT_EQUAL_UINT8(0xC8, e[1]);
      TEST_ASSERT_EQUAL_UINT8(0x37, e[2]);
    } else {
      TEST_ASSERT_EQUAL_MEMORY(kBlack, mac, 6);
      TEST_ASSERT_EQUAL_UINT8(0, e[1]);
    }
  }
}

void test_state_slot_without_w_caches_w_zero() {
  LampRoster roster;
  fill(true);
  roster.cacheWispState(gSlot, 1000);
  fill(false);
  roster.cacheWispState(gSlot, 2000);

  uint8_t blob[1 + 2 * WispFleetCache::kBlobEntrySize];
  roster.buildWispClaimsBlob(blob, sizeof(blob), 2000);
  const size_t ext = 1 + 2 * 12;
  for (size_t i = 0; i < 2; ++i) {
    TEST_ASSERT_EQUAL_UINT8(1, blob[ext + i * 3]);
    TEST_ASSERT_EQUAL_UINT8(0, blob[ext + i * 3 + 1]);
    TEST_ASSERT_EQUAL_UINT8(0, blob[ext + i * 3 + 2]);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_state_slot_reaches_claims_blob);
  RUN_TEST(test_state_slot_without_w_caches_w_zero);
  return UNITY_END();
}
