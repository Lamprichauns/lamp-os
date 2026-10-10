#include "config/disposition_store.hpp"

#include <Arduino.h>
#include <ArduinoJson.h>

#include <algorithm>
#include <array>

#include "util/bd_addr.hpp"

namespace lamp {

namespace {
using MacKey = std::array<uint8_t, 6>;
using Entry = std::pair<MacKey, uint8_t>;

inline MacKey keyOf(const uint8_t mac[6]) {
  MacKey k;
  std::copy(mac, mac + 6, k.begin());
  return k;
}

// Comparator for std::lower_bound over the sorted dispositions vector.
// Compares an existing entry's key against the target lookup key.
inline bool entryLess(const Entry& a, const MacKey& b) { return a.first < b; }

// Parse a JSON object of colon-hex mac -> disposition into a fresh key-sorted,
// deduped, clamped vector. Shared by load() and setFromJson(). parseBdAddr both
// validates and unpacks the key; legacy name-keyed entries (older firmware)
// fail it and are silently dropped, so the next write rewrites the NVS blob.
bool parseEntries(const char* json, size_t len, std::vector<Entry>& out) {
  JsonDocument doc;
  if (deserializeJson(doc, json, len) != DeserializationError::Ok) {
    return false;
  }
  if (!doc.is<JsonObject>()) return false;
  out.clear();
  out.reserve(DispositionStore::kMax);
  for (JsonPair kv : doc.as<JsonObject>()) {
    if (out.size() >= DispositionStore::kMax) break;
    MacKey mac;
    if (!parseBdAddr(kv.key().c_str(), mac.data())) continue;
    uint32_t v = kv.value() | (uint32_t)DispositionStore::kDefault;
    if (v < 1) v = 1;
    if (v > 5) v = 5;
    out.emplace_back(mac, static_cast<uint8_t>(v));
  }
  // Sort once (O(N log N) vs the O(N^2) of an N-call set() loop, each
  // shifting the tail). Byte order == canonical colon-hex string order.
  std::sort(out.begin(), out.end(),
            [](const Entry& a, const Entry& b) { return a.first < b.first; });
  // Dedupe: ArduinoJson is permissive on malformed input and may emit
  // duplicate keys. Last write wins.
  auto last = std::unique(
      out.begin(), out.end(),
      [](const Entry& a, const Entry& b) { return a.first == b.first; });
  out.erase(last, out.end());
  return true;
}
// Binary form: a format byte, then [mac:6][value:1] per entry, in key order.
bool parseBlob(const uint8_t* data, size_t len, std::vector<Entry>& out) {
  if (data[0] != DispositionStore::kBlobFormat) return false;
  if ((len - 1) % DispositionStore::kBlobEntryLen != 0) return false;
  const size_t count = (len - 1) / DispositionStore::kBlobEntryLen;
  if (count > DispositionStore::kMax) return false;
  out.clear();
  out.reserve(DispositionStore::kMax);
  for (size_t i = 0; i < count; ++i) {
    const uint8_t* e = data + 1 + i * DispositionStore::kBlobEntryLen;
    MacKey mac;
    std::copy(e, e + 6, mac.begin());
    out.emplace_back(mac, std::clamp<uint8_t>(e[6], 1, 5));
  }
  std::sort(out.begin(), out.end(),
            [](const Entry& a, const Entry& b) { return a.first < b.first; });
  return true;
}
}  // namespace

DispositionStore::DispositionStore(uint32_t flushIdleMs)
    : debouncer_(flushIdleMs) {
  mutex_ = xSemaphoreCreateMutex();
}

void DispositionStore::load() {
  if (!store_) return;
  std::array<uint8_t, kMaxBlobLen> blob;
  const size_t n = store_->readBlob(kBlobKey, blob.data(), blob.size());
  std::vector<Entry> next;
  if (n > 0) {
    if (!parseBlob(blob.data(), n, next)) return;
  } else {
    std::string json = store_->read(kLegacyJsonKey, "{}");
    if (!parseEntries(json.c_str(), json.size(), next)) return;
  }
  xSemaphoreTake(mutex_, portMAX_DELAY);
  entries_ = std::move(next);
  xSemaphoreGive(mutex_);
}

uint8_t DispositionStore::get(const uint8_t mac[6]) const {
  const MacKey key = keyOf(mac);
  // Binary search on the sorted vector. lower_bound returns the first entry
  // >= key; still compare keys because it can land on a strictly-greater
  // neighbour.
  xSemaphoreTake(mutex_, portMAX_DELAY);
  auto it = std::lower_bound(entries_.begin(), entries_.end(), key, entryLess);
  uint8_t value = (it == entries_.end() || it->first != key) ? kDefault
                                                             : it->second;
  xSemaphoreGive(mutex_);
  return value;
}

void DispositionStore::set(const uint8_t mac[6], uint8_t value,
                           uint32_t nowMs) {
  const MacKey key = keyOf(mac);
  if (value < 1) value = 1;
  if (value > 5) value = 5;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  auto it = std::lower_bound(entries_.begin(), entries_.end(), key, entryLess);
  if (it != entries_.end() && it->first == key) {
    // Update in place. No resize, no shift; sort order preserved.
    it->second = value;
  } else {
    if (entries_.size() >= kMax) {
      // Evict the lowest-by-key entry. Best-effort at the cap; users typically
      // have <100 paired lamps.
      entries_.erase(entries_.begin());
      it = std::lower_bound(entries_.begin(), entries_.end(), key, entryLess);
    }
    entries_.insert(it, std::make_pair(key, value));
  }
  xSemaphoreGive(mutex_);
  // Defer the write (see DispositionDebouncer). The Core 1 drain flushes
  // once the slider goes idle; BLE disconnect force-flushes.
  debouncer_.markDirty(nowMs);
}

size_t DispositionStore::writeJson_(char* out) const {
  // The compact shape ArduinoJson emits for the same object, written directly
  // so a flush builds no document and never grows a string. Sorted-vector
  // order is ascending byte order, which is canonical colon-hex order.
  xSemaphoreTake(mutex_, portMAX_DELAY);
  char* p = out;
  *p++ = '{';
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (i) *p++ = ',';
    *p++ = '"';
    formatBdAddr(entries_[i].first.data(), p);
    p += kBdAddrLen;
    *p++ = '"';
    *p++ = ':';
    *p++ = static_cast<char>('0' + entries_[i].second);
  }
  *p++ = '}';
  *p = '\0';
  xSemaphoreGive(mutex_);
  return static_cast<size_t>(p - out);
}

std::string DispositionStore::asJson() const {
  std::string out(kMaxJsonLen, '\0');
  out.resize(writeJson_(out.data()));
  return out;
}

bool DispositionStore::setFromJson(const char* json, size_t len,
                                   uint32_t nowMs) {
  std::vector<Entry> next;
  if (!parseEntries(json, len, next)) return false;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  entries_ = std::move(next);
  xSemaphoreGive(mutex_);
  debouncer_.markDirty(nowMs);
  return true;
}

bool DispositionStore::persist_() {
  if (!store_) return false;
  // At most 701 bytes, so the stack holds it and a flush touches no heap.
  std::array<uint8_t, kMaxBlobLen> blob;
  blob[0] = kBlobFormat;
  size_t len = 1;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  for (const auto& e : entries_) {
    std::copy(e.first.begin(), e.first.end(), blob.data() + len);
    blob[len + 6] = e.second;
    len += kBlobEntryLen;
  }
  xSemaphoreGive(mutex_);
  if (store_->writeBlob(kBlobKey, blob.data(), len) == 0) return false;
  store_->erase(kLegacyJsonKey);
  return true;
}

void DispositionStore::flushIfDue_(uint32_t nowMs) {
  if (!retry_.due(nowMs)) return;
  if (persist_()) {
    debouncer_.clear();
    retry_.succeed();
  } else {
    retry_.fail(nowMs);
    Serial.printf("[nvs] dispositions flush failed; retry in %lus\n",
                  (unsigned long)(retry_.waitMs() / 1000));
  }
}

void DispositionStore::maybeFlush(uint32_t nowMs) {
  if (debouncer_.shouldFlush(nowMs)) flushIfDue_(nowMs);
}

void DispositionStore::flushNow(uint32_t nowMs) {
  if (debouncer_.dirty()) flushIfDue_(nowMs);
}

}  // namespace lamp
