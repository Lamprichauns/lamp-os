#include "expressions/cascade_scheduler.hpp"

#include <cstdio>
#include <cstring>

namespace lamp {

namespace {
constexpr char kDelayField[] = "\"delayMs\":";
}  // namespace

bool CascadeScheduler::begin(const char* json, size_t len, uint32_t staggerMs,
                             uint32_t nowMs) {
  cancel();
  if (len > kMaxPayloadLen) return false;
  std::memcpy(payload_.data(), json, len);
  payload_[len] = '\0';
  const char* field = std::strstr(payload_.data(), kDelayField);
  if (!field) return false;
  delayPos_ = static_cast<size_t>(field - payload_.data()) + sizeof(kDelayField) - 1;
  delayDigits_ = 0;
  while (delayPos_ + delayDigits_ < len &&
         payload_[delayPos_ + delayDigits_] >= '0' &&
         payload_[delayPos_ + delayDigits_] <= '9') {
    ++delayDigits_;
  }
  if (delayDigits_ == 0) return false;
  len_ = len;
  staggerMs_ = staggerMs;
  startMs_ = nowMs;
  batchesSent_ = 0;
  return true;
}

void CascadeScheduler::addPeer(const uint8_t mac[6]) {
  if (count_ >= kMaxPeers) return;
  std::memcpy(peers_[count_].data(), mac, 6);
  ++count_;
}

uint32_t CascadeScheduler::delayFor(size_t index, uint32_t staggerMs,
                                    uint32_t elapsedMs) {
  const uint64_t intended = static_cast<uint64_t>(index + 1) * staggerMs;
  const uint32_t capped =
      intended > kMaxDelayMs ? kMaxDelayMs : static_cast<uint32_t>(intended);
  return capped > elapsedMs ? capped - elapsedMs : 0;
}

bool CascadeScheduler::setDelay(uint32_t delayMs) {
  char digits[11];
  const size_t n = static_cast<size_t>(
      std::snprintf(digits, sizeof(digits), "%lu", static_cast<unsigned long>(delayMs)));
  const size_t newLen = len_ - delayDigits_ + n;
  if (newLen > kMaxPayloadLen) return false;
  const size_t tail = delayPos_ + delayDigits_;
  std::memmove(payload_.data() + delayPos_ + n, payload_.data() + tail, len_ - tail + 1);
  std::memcpy(payload_.data() + delayPos_, digits, n);
  len_ = newLen;
  delayDigits_ = n;
  return true;
}

}  // namespace lamp
