#pragma once

#include <cstddef>
#include <cstdint>

namespace lamp {

// Writes compact JSON straight into a caller's fixed buffer, or only counts
// it when the buffer is null, so a document is serialized with no heap: no
// JsonDocument, no growing string. Output is byte-identical to ArduinoJson's
// serializeJson for the same document (same escaping, same number and
// boolean forms), which test_json_writer pins.
class JsonWriter {
 public:
  // `buf` holds `cap` bytes including the NUL; null measures only.
  JsonWriter(char* buf, size_t cap) : buf_(buf), cap_(cap) {}

  void beginObject() { open('{'); }
  void endObject() { close('}'); }
  void beginArray() { open('['); }
  void endArray() { close(']'); }
  // An object member's key; the value call that follows writes no comma.
  void key(const char* k);

  void value(const char* s);
  void value(uint32_t n);
  void value(bool b);

  // Bytes written (or that would be), excluding the NUL.
  size_t length() const { return len_; }
  // False when a real buffer ran out; the output is then truncated.
  bool ok() const { return !overflow_; }
  // NUL-terminates a real buffer.
  void finish();

 private:
  static constexpr uint8_t kMaxDepth = 8;
  void open(char c);
  void close(char c);
  void separate();
  void put(char c);
  void putRaw(const char* s);

  char* buf_;
  size_t cap_;
  size_t len_ = 0;
  bool overflow_ = false;
  bool afterKey_ = false;
  uint8_t depth_ = 0;
  uint8_t hasItems_ = 0;  // bit d: the container at depth d already has a value
};

}  // namespace lamp
