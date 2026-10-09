// OscListener feeds OSC packets from USB serial (SLIP frames, handed in by
// SerialConsole) and UDP into WispController's OSC palette input. Loop task
// only. The UDP socket is bound on every interface (softAP and STA) while
// `poll(true)` and closed on `poll(false)`.

#pragma once

#include <cstddef>
#include <cstdint>

namespace wisp {

class WispController;

// ponytail: one fixed address; make it configurable when a second sender
// or role is wanted.
inline constexpr char kOscPaletteAddress[] =
    "/straylight/palette/primary/colours";
inline constexpr uint16_t kOscUdpPort = 9000;

class OscListener {
 public:
  explicit OscListener(WispController& controller) : controller_(controller) {}

  void poll(bool active, uint32_t nowMs);
  void handlePacket(const uint8_t* data, size_t len, const char* via);

 private:
  void open(uint32_t nowMs);
  void close();

  WispController& controller_;
  int fd_ = -1;
  uint32_t lastOpenAttemptMs_ = 0;
  bool attempted_ = false;
  uint8_t rx_[1500];
};

}  // namespace wisp
