#pragma once

#include <cstddef>

namespace wisp {

// Writes `name` into out as a DHCP hostname: lowercase letters, digits and
// single hyphens, no leading or trailing hyphen, at most cap - 1 characters.
// Falls back to `fallback` when nothing valid is left. cap must be > 0.
void toHostname(const char* name, const char* fallback, char* out,
                size_t cap);

}  // namespace wisp
