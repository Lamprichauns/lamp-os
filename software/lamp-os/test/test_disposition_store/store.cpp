// Native-host tests for DispositionStore: get/set/clamp/eviction, the JSON
// bulk replace, and a persist/reload round trip through an InMemoryConfigStore
// (real persistence logic, no flash).

#include <unity.h>

#include <array>
#include <cstdio>

#include "config/config_store.hpp"
#include "config/disposition_store.hpp"
#include "util/bd_addr.hpp"

// Native tests don't build src/, so compile the real implementation in.
#include "../../src/config/disposition_store.cpp"
#include "../../src/util/heap_guard.cpp"

using namespace lamp;

// The store keys on the raw 6-byte MAC; unpack a canonical colon-hex literal
// so the tests can keep reading like the on-wire form.
static std::array<uint8_t, 6> mac(const char* s) {
  std::array<uint8_t, 6> m{};
  TEST_ASSERT_TRUE(parseBdAddr(s, m.data()));
  return m;
}
static uint8_t getMac(const DispositionStore& ds, const char* s) {
  return ds.get(mac(s).data());
}
static void setMac(DispositionStore& ds, const char* s, uint8_t v, uint32_t t) {
  ds.set(mac(s).data(), v, t);
}

void setUp(void) { g_mock_largest_free_block = 1 << 20; }
void tearDown(void) {}

void test_unknown_peer_returns_default() {
  DispositionStore ds(100);
  TEST_ASSERT_EQUAL_UINT8(DispositionStore::kDefault, getMac(ds, "AA:BB:CC:DD:EE:FF"));
}

void test_keyed_by_mac() {
  DispositionStore ds(100);
  setMac(ds, "C4:DD:57:EB:64:60", 4, 0);
  TEST_ASSERT_EQUAL_UINT8(4, getMac(ds, "C4:DD:57:EB:64:60"));
}

void test_set_get_and_clamp() {
  DispositionStore ds(100);
  setMac(ds, "AA:BB:CC:DD:EE:01", 4, 0);
  TEST_ASSERT_EQUAL_UINT8(4, getMac(ds, "AA:BB:CC:DD:EE:01"));
  setMac(ds, "AA:BB:CC:DD:EE:02", 9, 0);  // clamps to 5
  TEST_ASSERT_EQUAL_UINT8(5, getMac(ds, "AA:BB:CC:DD:EE:02"));
  setMac(ds, "AA:BB:CC:DD:EE:03", 0, 0);  // clamps to 1
  TEST_ASSERT_EQUAL_UINT8(1, getMac(ds, "AA:BB:CC:DD:EE:03"));
}

void test_eviction_at_capacity_drops_lowest_key() {
  DispositionStore ds(100);
  char buf[18];
  for (int i = 0; i < 100; i++) {
    snprintf(buf, sizeof(buf), "AA:BB:CC:DD:EE:%02X", i);
    setMac(ds, buf, 2, 0);
  }
  setMac(ds, "AA:BB:CC:DD:EE:00", 1, 0);  // distinct value on the lowest key
  // New key at capacity → evict the lowest-by-key entry (..:00).
  setMac(ds, "FF:FF:FF:FF:FF:FF", 5, 0);
  TEST_ASSERT_EQUAL_UINT8(DispositionStore::kDefault,
                          getMac(ds, "AA:BB:CC:DD:EE:00"));  // evicted
  TEST_ASSERT_EQUAL_UINT8(2, getMac(ds, "AA:BB:CC:DD:EE:01"));  // kept
  TEST_ASSERT_EQUAL_UINT8(5, getMac(ds, "FF:FF:FF:FF:FF:FF"));  // inserted
}

void test_set_from_json_filters_invalid_keys() {
  DispositionStore ds(100);
  const char* json =
      "{\"AA:BB:CC:DD:EE:01\":4,\"not-a-bdaddr\":2,\"AA:BB:CC:DD:EE:02\":5}";
  TEST_ASSERT_TRUE(ds.setFromJson(json, std::string(json).size(), 0));
  TEST_ASSERT_EQUAL_UINT8(4, getMac(ds, "AA:BB:CC:DD:EE:01"));
  TEST_ASSERT_EQUAL_UINT8(5, getMac(ds, "AA:BB:CC:DD:EE:02"));
}

void test_debounced_persist_and_reload_round_trip() {
  InMemoryConfigStore backing;
  DispositionStore ds(100);  // 100 ms idle window
  ds.attachStore(&backing);

  setMac(ds, "AA:BB:CC:DD:EE:01", 4, 1000);
  ds.maybeFlush(1050);  // within window → no write yet
  TEST_ASSERT_EQUAL_STRING("MISS", backing.read("dispositions", "MISS").c_str());
  ds.maybeFlush(1100);  // window elapsed → writes
  TEST_ASSERT_TRUE(backing.read("dispositions", "MISS") != "MISS");

  // A fresh store over the same backing loads the persisted value.
  DispositionStore reloaded(100);
  reloaded.attachStore(&backing);
  reloaded.load();
  TEST_ASSERT_EQUAL_UINT8(4, getMac(reloaded, "AA:BB:CC:DD:EE:01"));
}

void test_flush_now_writes_immediately() {
  InMemoryConfigStore backing;
  DispositionStore ds(100000);  // long window; only flushNow should write
  ds.attachStore(&backing);
  setMac(ds, "AA:BB:CC:DD:EE:09", 3, 0);
  ds.flushNow();
  TEST_ASSERT_TRUE(backing.read("dispositions", "MISS") != "MISS");
}

// The on-disk JSON must stay byte-identical across the byte-key packing:
// canonical uppercase colon-hex keys, ascending, compact. Byte order of the
// packed MAC matches the old string order, so the ':' separator (0x3A, between
// '9' and 'A') and per-byte hex ordering are preserved.
void test_asjson_is_sorted_canonical_colon_hex() {
  DispositionStore ds(100);
  setMac(ds, "FF:EE:DD:CC:BB:AA", 5, 0);
  setMac(ds, "AA:BB:CC:DD:EE:09", 2, 0);
  setMac(ds, "AA:BB:CC:DD:EE:0A", 4, 0);  // 0x0A sorts after 0x09
  TEST_ASSERT_EQUAL_STRING(
      "{\"AA:BB:CC:DD:EE:09\":2,\"AA:BB:CC:DD:EE:0A\":4,\"FF:EE:DD:CC:BB:AA\":5}",
      ds.asJson().c_str());
}

// An existing persisted hex-string blob (as older firmware wrote it) loads
// into the byte-keyed store with every disposition preserved, and re-serializes
// to the same canonical bytes — proving the upgrade keeps learned dispositions.
void test_loads_existing_persisted_blob() {
  InMemoryConfigStore backing;
  backing.write("dispositions",
                "{\"11:22:33:44:55:66\":1,\"C4:DD:57:EB:64:60\":4}");
  DispositionStore ds(100);
  ds.attachStore(&backing);
  ds.load();
  TEST_ASSERT_EQUAL_UINT8(1, getMac(ds, "11:22:33:44:55:66"));
  TEST_ASSERT_EQUAL_UINT8(4, getMac(ds, "C4:DD:57:EB:64:60"));
  TEST_ASSERT_EQUAL_STRING(
      "{\"11:22:33:44:55:66\":1,\"C4:DD:57:EB:64:60\":4}", ds.asJson().c_str());
}

// A fragmented heap defers the flush instead of aborting in a growing string;
// the entries stay dirty and land once a later window finds the room.
void test_flush_deferred_on_low_heap_then_retried() {
  InMemoryConfigStore backing;
  DispositionStore ds(100);
  ds.attachStore(&backing);
  setMac(ds, "AA:BB:CC:DD:EE:01", 5, 0);
  g_mock_largest_free_block = 1024;
  ds.maybeFlush(100);
  TEST_ASSERT_EQUAL_STRING("MISS", backing.read("dispositions", "MISS").c_str());
  g_mock_largest_free_block = 1 << 20;
  ds.maybeFlush(150);  // inside the re-armed window: no write yet
  TEST_ASSERT_EQUAL_STRING("MISS", backing.read("dispositions", "MISS").c_str());
  ds.maybeFlush(200);
  TEST_ASSERT_EQUAL_STRING("{\"AA:BB:CC:DD:EE:01\":5}",
                           backing.read("dispositions", "MISS").c_str());
}

// The direct writer emits exactly what ArduinoJson serialized for the same
// object, at full capacity, so a stored blob reads back unchanged.
void test_full_store_matches_arduinojson_bytes() {
  DispositionStore ds(100);
  JsonDocument doc;
  char key[18];
  for (unsigned i = 0; i < DispositionStore::kMax; ++i) {
    const uint8_t m[6] = {0x10, 0x20, 0x30, 0x40, static_cast<uint8_t>(i >> 8),
                          static_cast<uint8_t>(i)};
    const uint8_t v = static_cast<uint8_t>(1 + i % 5);
    ds.set(m, v, 0);
    formatBdAddr(m, key);
    doc[key] = v;
  }
  std::string expected;
  serializeJson(doc, expected);
  TEST_ASSERT_EQUAL_STRING(expected.c_str(), ds.asJson().c_str());
}

void test_empty_store_serializes_empty_object() {
  DispositionStore ds(100);
  TEST_ASSERT_EQUAL_STRING("{}", ds.asJson().c_str());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_unknown_peer_returns_default);
  RUN_TEST(test_keyed_by_mac);
  RUN_TEST(test_set_get_and_clamp);
  RUN_TEST(test_eviction_at_capacity_drops_lowest_key);
  RUN_TEST(test_set_from_json_filters_invalid_keys);
  RUN_TEST(test_debounced_persist_and_reload_round_trip);
  RUN_TEST(test_flush_now_writes_immediately);
  RUN_TEST(test_asjson_is_sorted_canonical_colon_hex);
  RUN_TEST(test_loads_existing_persisted_blob);
  RUN_TEST(test_flush_deferred_on_low_heap_then_retried);
  RUN_TEST(test_full_store_matches_arduinojson_bytes);
  RUN_TEST(test_empty_store_serializes_empty_object);
  return UNITY_END();
}
