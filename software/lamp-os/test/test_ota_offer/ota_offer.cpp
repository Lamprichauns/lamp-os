// Pins the offer scan's pacing and its receive-first gate.

#include <unity.h>
#include <cstdint>
#include <vector>

#include "components/firmware/ota_offer.hpp"  // -I src on native

using lamp::MeshPeer;
using lamp::OtaOfferBusy;
using lamp::OtaOfferThrottle;
using lamp::offerBusy;
using lamp::peerRunsNewerFirmware;

void setUp(void) {}
void tearDown(void) {}

void test_first_idle_tick_scans() {
  OtaOfferThrottle t;
  TEST_ASSERT_TRUE(t.due({}, 0));
}

void test_scans_once_per_interval() {
  OtaOfferThrottle t;
  TEST_ASSERT_TRUE(t.due({}, 5000));
  TEST_ASSERT_FALSE(t.due({}, 5001));
  TEST_ASSERT_FALSE(t.due({}, 5000 + OtaOfferThrottle::kIntervalMs - 1));
  TEST_ASSERT_TRUE(t.due({}, 5000 + OtaOfferThrottle::kIntervalMs));
}

void test_interval_survives_millis_wrap() {
  OtaOfferThrottle t;
  TEST_ASSERT_TRUE(t.due({}, 0xFFFFFE00u));
  TEST_ASSERT_FALSE(t.due({}, 0xFFFFFF00u));
  TEST_ASSERT_FALSE(t.due({}, 0x00000010u));
  TEST_ASSERT_TRUE(t.due({}, 0xFFFFFE00u + OtaOfferThrottle::kIntervalMs));
}

void test_long_busy_spell_never_locks_the_scan_out() {
  OtaOfferThrottle t;
  TEST_ASSERT_TRUE(t.due({}, 1000));
  TEST_ASSERT_TRUE(t.due({}, 1000 + 0x80000000u));
}

void test_each_busy_source_blocks_the_scan() {
  OtaOfferBusy sending;   sending.firmwareSending = true;
  OtaOfferBusy receiving; receiving.firmwareReceiving = true;
  OtaOfferBusy fs;        fs.fsBusy = true;
  OtaOfferBusy quiet;     quiet.quiet = true;
  for (const OtaOfferBusy& b : {sending, receiving, fs, quiet}) {
    OtaOfferThrottle t;
    TEST_ASSERT_FALSE(t.due(b, 5000));
  }
}

void test_startup_phase_holds_offers() {
  OtaOfferThrottle t;
  OtaOfferBusy startingUp;
  startingUp.startingUp = true;
  TEST_ASSERT_FALSE(t.due(startingUp, 5000));
  TEST_ASSERT_TRUE(t.due({}, 5001));
}

void test_offer_busy_holds_until_startup_completes() {
  TEST_ASSERT_TRUE(offerBusy(/*startupComplete=*/false, false, false, false, false).any());
  TEST_ASSERT_FALSE(offerBusy(/*startupComplete=*/true, false, false, false, false).any());
}

void test_offer_busy_maps_each_source() {
  TEST_ASSERT_TRUE(offerBusy(true, true, false, false, false).firmwareSending);
  TEST_ASSERT_TRUE(offerBusy(true, false, true, false, false).firmwareReceiving);
  TEST_ASSERT_TRUE(offerBusy(true, false, false, true, false).fsBusy);
  TEST_ASSERT_TRUE(offerBusy(true, false, false, false, true).quiet);
}

void test_busy_tick_does_not_spend_the_interval() {
  OtaOfferThrottle t;
  TEST_ASSERT_TRUE(t.due({}, 5000));
  OtaOfferBusy receiving;
  receiving.firmwareReceiving = true;
  TEST_ASSERT_FALSE(t.due(receiving, 5000 + OtaOfferThrottle::kIntervalMs));
  TEST_ASSERT_TRUE(t.due({}, 5000 + OtaOfferThrottle::kIntervalMs + 1));
}

MeshPeer peer(uint32_t version, bool hasMac = true) {
  MeshPeer p;
  p.firmwareVersion = version;
  p.hasMac = hasMac;
  return p;
}

void test_newer_peer_holds_offers() {
  std::vector<MeshPeer> peers{peer(0x00010001u), peer(0x00010003u)};
  TEST_ASSERT_TRUE(peerRunsNewerFirmware(peers, 0x00010002u));
}

void test_same_or_older_peers_allow_offers() {
  std::vector<MeshPeer> peers{peer(0x00010001u), peer(0x00010002u)};
  TEST_ASSERT_FALSE(peerRunsNewerFirmware(peers, 0x00010002u));
  TEST_ASSERT_FALSE(peerRunsNewerFirmware({}, 0x00010002u));
}

void test_peer_without_mac_or_version_is_ignored() {
  std::vector<MeshPeer> peers{peer(0x00010003u, /*hasMac=*/false), peer(0)};
  TEST_ASSERT_FALSE(peerRunsNewerFirmware(peers, 0x00010002u));
}

int main(int argc, char** argv) {
  (void)argc; (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_first_idle_tick_scans);
  RUN_TEST(test_scans_once_per_interval);
  RUN_TEST(test_interval_survives_millis_wrap);
  RUN_TEST(test_long_busy_spell_never_locks_the_scan_out);
  RUN_TEST(test_each_busy_source_blocks_the_scan);
  RUN_TEST(test_startup_phase_holds_offers);
  RUN_TEST(test_offer_busy_holds_until_startup_completes);
  RUN_TEST(test_offer_busy_maps_each_source);
  RUN_TEST(test_busy_tick_does_not_spend_the_interval);
  RUN_TEST(test_newer_peer_holds_offers);
  RUN_TEST(test_same_or_older_peers_allow_offers);
  RUN_TEST(test_peer_without_mac_or_version_is_ignored);
  return UNITY_END();
}
