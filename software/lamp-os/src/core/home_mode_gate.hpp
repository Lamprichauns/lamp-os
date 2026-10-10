#pragma once

#include <string>
#include <vector>

#include "expressions/transient_admission.hpp"

namespace lamp {

// Pure membership test for the compositor home-mode skip decision.
// No Compositor or AnimatedBehavior deps, usable in native tests directly.
inline bool homeModeExpressionSkips(const char* id,
                                    const std::vector<std::string>& disabled) {
  if (!id) return false;
  for (const auto& s : disabled) {
    if (s == id) return true;
  }
  return false;
}

// True when home mode skips a social behavior: social is off and the behavior
// is not playing a greeting the app asked for (the owner's own test button).
constexpr bool homeModeSkipsSocial(bool socialDisabled, bool appGreeting) {
  return socialDisabled && !appGreeting;
}

// True when this lamp ignores what `source` asks it to draw: home mode is
// active with social off and the request came from another lamp. Every peer
// path (cascades, event mirrors, arrivals, nearby peers, crowd-dim) and the
// reaping of peer transients already running route through this one decision.
constexpr bool homeModeBlocksRemote(bool homeActive, bool socialDisabled,
                                    TransientSource source) {
  return homeActive && socialDisabled && source == TransientSource::Remote;
}

}  // namespace lamp
