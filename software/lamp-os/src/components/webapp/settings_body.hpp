#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace webapp {

// The settings PUT body's limit. A lamp at every cap sends about 12.9 KB.
constexpr size_t kMaxSettingsBody = 16 * 1024;

// Gathers a request body that arrives in pieces (`index` its offset, `total`
// the declared length) into one calloc'd, NUL-terminated block in `slot`, the
// request's own pointer. A body over `maxBody` is never gathered, nor one
// whose block `canSpare` refuses; a piece that runs past `total` (a body with
// no declared length included) frees what was gathered. Host-testable: the handler
// passes the request's _tempObject and heapCanSpare.
inline void collectBodyPiece(void*& slot, const uint8_t* data, size_t len, size_t index,
                             size_t total, size_t maxBody, bool (*canSpare)(size_t)) {
  if (total > maxBody) return;
  if (index == 0 && slot == nullptr && canSpare(total + 1)) slot = std::calloc(total + 1, 1);
  if (slot == nullptr) return;
  if (index + len > total) {
    std::free(slot);
    slot = nullptr;
    return;
  }
  std::memcpy(static_cast<char*>(slot) + index, data, len);
}

// The HTTP status to answer before handing the body on, or 0 to hand it on:
// 415 for a body that is not JSON, 413 over the limit, 400 with no body,
// 500 when the gathered block is missing (the heap could not spare it, or a
// piece overran the declared length).
inline int settingsBodyStatus(bool isJson, size_t contentLength, const void* slot,
                              size_t maxBody) {
  if (!isJson) return 415;
  if (contentLength > maxBody) return 413;
  if (contentLength == 0) return 400;
  if (slot == nullptr) return 500;
  return 0;
}

// Moves the gathered block out of the request, so the request no longer
// frees it; the caller owns it and frees it with free().
inline char* takeBody(void*& slot) {
  char* body = static_cast<char*>(slot);
  slot = nullptr;
  return body;
}

}  // namespace webapp
