#include "expressions/cascade_scheduler.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace lamp {

namespace {
constexpr char kDelayField[] = "\"delayMs\":";
}  // namespace

CascadeScheduler::Begin CascadeScheduler::begin(const char* json, size_t len,
                                                uint32_t staggerMs, uint32_t nowMs) {
  fillingValid_ = false;
  if (len > kMaxPayloadLen) return Begin::Refused;
  // Locate the delay digits before touching a slot, so a refusal holds
  // nothing new.
  const char* end = json + len;
  const char* field = std::search(json, end, kDelayField, kDelayField + sizeof(kDelayField) - 1);
  if (field == end) return Begin::Refused;
  const size_t delayPos = static_cast<size_t>(field - json) + sizeof(kDelayField) - 1;
  size_t delayDigits = 0;
  while (delayPos + delayDigits < len && json[delayPos + delayDigits] >= '0' &&
         json[delayPos + delayDigits] <= '9') {
    ++delayDigits;
  }
  if (delayDigits == 0) return Begin::Refused;

  Begin result = Begin::Queued;
  size_t slot;
  if (count_ < kMaxDrains) {
    slot = (head_ + count_) % kMaxDrains;
    ++count_;
  } else {
    // Full: the draining fan-out keeps going; the one waiting last yields.
    slot = (head_ + count_ - 1) % kMaxDrains;
    result = Begin::Replaced;
  }
  Drain& d = drains_[slot];
  d = Drain{};
  std::memcpy(d.payload.data(), json, len);
  d.payload[len] = '\0';
  d.len = len;
  d.delayPos = delayPos;
  d.delayDigits = delayDigits;
  d.staggerMs = staggerMs;
  d.startMs = nowMs;
  filling_ = slot;
  fillingValid_ = true;
  return result;
}

void CascadeScheduler::addPeer(const uint8_t mac[6]) {
  if (!fillingValid_) return;
  Drain& d = drains_[filling_];
  if (d.count >= kMaxPeers) return;
  std::memcpy(d.peers[d.count].data(), mac, 6);
  ++d.count;
}

void CascadeScheduler::cancel() {
  head_ = count_ = 0;
  fillingValid_ = false;
}

size_t CascadeScheduler::pending() const {
  size_t n = 0;
  for (size_t i = 0; i < count_; ++i) {
    const Drain& d = drains_[(head_ + i) % kMaxDrains];
    n += d.count - d.next;
  }
  return n;
}

void CascadeScheduler::pop() {
  if (fillingValid_ && filling_ == head_) fillingValid_ = false;
  head_ = (head_ + 1) % kMaxDrains;
  --count_;
}

uint32_t CascadeScheduler::delayFor(size_t index, uint32_t staggerMs,
                                    uint32_t elapsedMs) {
  const uint64_t intended = static_cast<uint64_t>(index + 1) * staggerMs;
  const uint32_t capped =
      intended > kMaxDelayMs ? kMaxDelayMs : static_cast<uint32_t>(intended);
  return capped > elapsedMs ? capped - elapsedMs : 0;
}

bool CascadeScheduler::setDelay(Drain& d, uint32_t delayMs) {
  char digits[11];
  const size_t n = static_cast<size_t>(
      std::snprintf(digits, sizeof(digits), "%lu", static_cast<unsigned long>(delayMs)));
  const size_t newLen = d.len - d.delayDigits + n;
  if (newLen > kMaxPayloadLen) return false;
  const size_t tail = d.delayPos + d.delayDigits;
  std::memmove(d.payload.data() + d.delayPos + n, d.payload.data() + tail, d.len - tail + 1);
  std::memcpy(d.payload.data() + d.delayPos, digits, n);
  d.len = newLen;
  d.delayDigits = n;
  return true;
}

}  // namespace lamp
