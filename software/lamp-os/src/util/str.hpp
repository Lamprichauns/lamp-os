#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace lamp {

// Canonical storage form for user-facing names: ASCII-lowercase. Display
// edges (app, web UI) title-case for presentation.
inline std::string asciiLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

// Canonical stored lamp name. Whitespace is dropped, not just trimmed: the
// name composes the setup AP's SSID, the BLE advert name and the mDNS host,
// and a space in any of those leaves an owner hunting for a network that
// does not match the name their app shows.
inline std::string normalizeLampName(std::string s) {
  s.erase(std::remove_if(s.begin(), s.end(),
                         [](unsigned char c) { return std::isspace(c); }),
          s.end());
  return asciiLower(std::move(s));
}

}  // namespace lamp
