// Native tests for gathering the settings PUT body: pieces assemble into one
// block, an over-limit, undeclared or overrunning body and a refused block
// are never handed on, and the hand-off moves ownership out of the request.

#include <unity.h>

#include <algorithm>
#include <string>

#include "components/webapp/settings_body.hpp"

using namespace webapp;

void setUp(void) {}
void tearDown(void) {}

namespace {
bool spare(size_t) { return true; }
bool noSpare(size_t) { return false; }
constexpr size_t kMax = 64;

void feed(void*& slot, const std::string& body, size_t piece, size_t total,
          bool (*canSpare)(size_t) = spare) {
  for (size_t at = 0; at < body.size(); at += piece) {
    const size_t len = std::min(piece, body.size() - at);
    collectBodyPiece(slot, reinterpret_cast<const uint8_t*>(body.data()) + at, len, at, total,
                     kMax, canSpare);
  }
}
}  // namespace

void test_pieces_assemble_into_one_terminated_block() {
  void* slot = nullptr;
  const std::string body = R"({"lamp":{"name":"jacko"}})";
  feed(slot, body, 7, body.size());
  TEST_ASSERT_NOT_NULL(slot);
  TEST_ASSERT_EQUAL_STRING(body.c_str(), static_cast<char*>(slot));
  TEST_ASSERT_EQUAL(0, settingsBodyStatus(true, body.size(), slot, kMax));
  char* taken = takeBody(slot);
  TEST_ASSERT_NULL(slot);
  TEST_ASSERT_EQUAL_STRING(body.c_str(), taken);
  std::free(taken);
}

void test_an_over_limit_body_is_never_gathered() {
  void* slot = nullptr;
  const std::string body(kMax + 1, 'x');
  feed(slot, body, 16, body.size());
  TEST_ASSERT_NULL(slot);
  TEST_ASSERT_EQUAL(413, settingsBodyStatus(true, body.size(), slot, kMax));
}

void test_a_body_with_no_declared_length_is_never_gathered() {
  void* slot = nullptr;
  feed(slot, "{}", 2, 0);
  TEST_ASSERT_NULL(slot);
  TEST_ASSERT_EQUAL(400, settingsBodyStatus(true, 0, slot, kMax));
}

void test_a_piece_past_the_declared_length_drops_the_body() {
  void* slot = nullptr;
  feed(slot, "{\"lamp\":{}}", 4, 6);
  TEST_ASSERT_NULL(slot);
  TEST_ASSERT_EQUAL(500, settingsBodyStatus(true, 6, slot, kMax));
}

void test_a_refused_block_is_never_gathered() {
  void* slot = nullptr;
  feed(slot, "{}", 1, 2, noSpare);
  TEST_ASSERT_NULL(slot);
  TEST_ASSERT_EQUAL(500, settingsBodyStatus(true, 2, slot, kMax));
}

void test_a_body_that_is_not_json_is_refused() {
  void* slot = nullptr;
  feed(slot, "{}", 2, 2);
  TEST_ASSERT_EQUAL(415, settingsBodyStatus(false, 2, slot, kMax));
  std::free(takeBody(slot));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pieces_assemble_into_one_terminated_block);
  RUN_TEST(test_an_over_limit_body_is_never_gathered);
  RUN_TEST(test_a_body_with_no_declared_length_is_never_gathered);
  RUN_TEST(test_a_piece_past_the_declared_length_drops_the_body);
  RUN_TEST(test_a_refused_block_is_never_gathered);
  RUN_TEST(test_a_body_that_is_not_json_is_refused);
  return UNITY_END();
}
