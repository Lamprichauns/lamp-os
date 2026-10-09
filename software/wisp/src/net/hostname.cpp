#include "net/hostname.hpp"

#include <cstdio>

namespace wisp {

void toHostname(const char* name, const char* fallback, char* out,
                size_t cap) {
  size_t n = 0;
  bool pendingHyphen = false;
  for (const char* p = name; p && *p && n + 1 < cap; ++p) {
    char c = *p;
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    const bool valid = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if (!valid) {
      pendingHyphen = n > 0;
      continue;
    }
    if (pendingHyphen) {
      if (n + 2 >= cap) break;
      out[n++] = '-';
      pendingHyphen = false;
    }
    out[n++] = c;
  }
  out[n] = '\0';
  if (n == 0) std::snprintf(out, cap, "%s", fallback);
}

}  // namespace wisp
