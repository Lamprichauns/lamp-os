#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace lamp {

enum class WispBroadcast { None, Status, Config };

// Classify an inbound CONTROL_OP payload on the WiFi task so the wisp's
// status and config broadcasts land in their own pending slots and never
// displace another op from the single inbound slot. The wisp serializes
// "char" first; a payload that starts any other way takes the inbound slot,
// whose drain classifies it with wispBroadcastTag.
inline WispBroadcast peekWispBroadcast(const uint8_t* payload, size_t len) {
  static constexpr char kStatus[] = R"({"char":"wispStatus")";
  static constexpr char kConfig[] = R"({"char":"wispConfig")";
  const auto startsWith = [&](const char* prefix, size_t n) {
    return len >= n && std::memcmp(payload, prefix, n) == 0;
  };
  if (startsWith(kStatus, sizeof(kStatus) - 1)) return WispBroadcast::Status;
  if (startsWith(kConfig, sizeof(kConfig) - 1)) return WispBroadcast::Config;
  return WispBroadcast::None;
}

// Classify a parsed CONTROL_OP "char" tag. The wisp's status and config are
// the only control ops a lamp takes from the mesh; every other tag is
// relayed and dropped.
inline WispBroadcast wispBroadcastTag(const char* tag) {
  if (!tag) return WispBroadcast::None;
  if (std::strcmp(tag, "wispStatus") == 0) return WispBroadcast::Status;
  if (std::strcmp(tag, "wispConfig") == 0) return WispBroadcast::Config;
  return WispBroadcast::None;
}

}  // namespace lamp
