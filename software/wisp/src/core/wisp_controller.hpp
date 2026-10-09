// WispController is the source-mode orchestration brain.
//
// Owns the mode-transition, manual-palette push, OSC palette input and
// stream-liveness handling, and the op-result application invoked by
// MeshRouter after a CONTROL_OP dispatch. Triggers the status-ring render.
// Loop task only.

#pragma once

#include "paint/current_palette.hpp"  // CurrentPalette, Palette
#include "config/wisp_op_dispatcher.hpp"  // DispatchResult
#include "osc/osc_freshness.hpp"
#include "status/status_ring.hpp"  // kMaxRingPixels

class Adafruit_NeoPixel;

namespace wisp {

class PaintDistributor;
class WispConfig;
class StatusEmitter;
class ArtnetEmitter;
enum class WispSourceMode : uint8_t;

class WispController {
 public:
  WispController(CurrentPalette& palette, PaintDistributor& paint,
                 WispConfig& config, StatusEmitter& status,
                 ArtnetEmitter& artnet, Adafruit_NeoPixel& strip)
      : palette_(palette),
        paint_(paint),
        config_(config),
        status_(status),
        artnet_(artnet),
        strip_(strip) {}

  // Idempotent: safe at boot (from NVS) and on every mode flip.
  void applySourceModeTransition(WispSourceMode mode);

  // Apply the DispatchResult from a CONTROL_OP. MeshRouter result callback.
  void applyOpResult(DispatchResult res);

  // OSC palette input (flat RGB floats 0..1). Applied only in Osc mode;
  // an unchanged palette only refreshes liveness. Never changes paint mode.
  void onOscPalette(const float* rgb, size_t colours, const char* via,
                    uint32_t nowMs);

  // Osc mode with no palette for OscFreshness::kStaleMs: hold lamps and ring
  // at Off with paint mode kept, until the next palette.
  void tickOscLiveness(uint32_t now);

  // Osc mode with a fresh palette. HELLO flag bit 2 + wispStatus sourceLive.
  bool sourceLive() const;

  // Re-init the strip from the current LED format + pixel count config.
  // Call once after config load; call again after setLedStrip op.
  void applyLedConfig();

  // Loop-task driver for the own-ring Off<->painting crossfade. No-op when idle.
  void tickRingFade(uint32_t now);

 private:
  bool effectiveOff() const;
  void pushManualPaletteToCurrent();
  void renderRing();
  void computeRingPixels(uint8_t* pixels, size_t px, bool off);
  void showPixels(const uint8_t* pixels, size_t px);

  CurrentPalette& palette_;
  PaintDistributor& paint_;
  WispConfig& config_;
  StatusEmitter& status_;
  ArtnetEmitter& artnet_;
  Adafruit_NeoPixel& strip_;

  void resetOscStream();

  static constexpr size_t kOscMaxFloats = 150;  // kManualPaletteMaxColors * 3
  OscFreshness oscFresh_;
  // Last applied palette; an identical one only refreshes liveness.
  float oscLast_[kOscMaxFloats] = {0};
  size_t oscLastFloats_ = 0;
  uint32_t oscLastLogMs_ = 0;
  // Reused across changes so a streaming palette never reallocates.
  Palette oscScratch_;

  // Own-ring crossfade state. shown_ mirrors the last emitted (pre-gamma)
  // frame so a fresh Off<->painting flip can blend out of whatever is lit.
  uint8_t shown_[kMaxRingPixels * 3] = {0};
  uint8_t fadeFrom_[kMaxRingPixels * 3] = {0};
  uint8_t fadeTo_[kMaxRingPixels * 3] = {0};
  size_t shownPx_ = 0;
  size_t fadePx_ = 0;
  uint32_t fadeStartMs_ = 0;
  bool fading_ = false;
  bool ringReady_ = false;
  bool lastOff_ = false;
};

}  // namespace wisp
