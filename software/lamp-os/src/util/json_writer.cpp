#include "util/json_writer.hpp"

#include <cstdio>

namespace lamp {

void JsonWriter::put(char c) {
  if (buf_) {
    if (len_ + 1 < cap_) {
      buf_[len_] = c;
    } else {
      overflow_ = true;
    }
  }
  ++len_;
}

void JsonWriter::putRaw(const char* s) {
  while (*s) put(*s++);
}

void JsonWriter::separate() {
  if (afterKey_) {
    afterKey_ = false;
    return;
  }
  if (depth_ == 0) return;
  const uint8_t bit = 1u << (depth_ - 1);
  if (hasItems_ & bit) put(',');
  hasItems_ |= bit;
}

void JsonWriter::open(char c) {
  separate();
  put(c);
  if (depth_ < kMaxDepth) {
    hasItems_ &= ~(1u << depth_);
    ++depth_;
  } else {
    overflow_ = true;
  }
}

void JsonWriter::close(char c) {
  put(c);
  if (depth_ > 0) --depth_;
}

void JsonWriter::key(const char* k) {
  value(k);
  put(':');
  afterKey_ = true;
}

void JsonWriter::value(const char* s) {
  separate();
  put('"');
  for (; *s; ++s) {
    switch (*s) {
      case '"': putRaw("\\\""); break;
      case '\\': putRaw("\\\\"); break;
      case '\b': putRaw("\\b"); break;
      case '\f': putRaw("\\f"); break;
      case '\n': putRaw("\\n"); break;
      case '\r': putRaw("\\r"); break;
      case '\t': putRaw("\\t"); break;
      default: put(*s);
    }
  }
  put('"');
}

void JsonWriter::value(uint32_t n) {
  separate();
  char digits[11];
  std::snprintf(digits, sizeof(digits), "%lu", static_cast<unsigned long>(n));
  putRaw(digits);
}

void JsonWriter::value(bool b) {
  separate();
  putRaw(b ? "true" : "false");
}

void JsonWriter::finish() {
  if (!buf_ || cap_ == 0) return;
  buf_[len_ < cap_ ? len_ : cap_ - 1] = '\0';
}

}  // namespace lamp
