#pragma once

#include <Arduino.h>
#include <functional>

#include "osc/slip_decoder.hpp"

namespace wisp {

class PaintDistributor;
class StageBeacon;
class WifiLink;
class StatusEmitter;
class LampInventory;
class OscListener;
class WispConfig;
enum class WispSourceMode : uint8_t;

class SerialConsole {
public:
  using SourceTransitionFn = std::function<void(WispSourceMode)>;

  SerialConsole(PaintDistributor& paint, WispConfig& config,
                StageBeacon& stage, WifiLink& wifi, StatusEmitter& status,
                LampInventory& inventory,
                SourceTransitionFn onSourceTransition);

  // Read available serial bytes; SLIP frames go to the OSC sink, everything
  // else dispatches as text command lines.
  void pump();

  void setOscSink(OscListener* osc) { osc_ = osc; }

  // Print the current lamp roster to Serial.
  void dumpInventory();

private:
  void handleCommand(const String& cmd);
  static String formatVersion(uint32_t v);

  PaintDistributor& paint_;
  WispConfig& config_;
  StageBeacon& stage_;
  WifiLink& wifi_;
  StatusEmitter& status_;
  LampInventory& inventory_;
  SourceTransitionFn onSourceTransition_;

  String buf_;
  OscListener* osc_ = nullptr;
  SlipDecoder slip_;
};

}  // namespace wisp
