// WispController is the source-mode + Aurora orchestration brain.
//
// Owns the mode-transition, manual-palette push, Aurora palette callback,
// Aurora-liveness edge handling, and the op-result application invoked by
// MeshRouter after a CONTROL_OP dispatch. Triggers the status-ring render.
// All methods run on the loop task except onAuroraPalette, which fires from
// auroraClient.loop() (also the loop task).

#pragma once

#include "paint/current_palette.hpp"  // CurrentPalette, Palette
#include "config/wisp_op_dispatcher.hpp"  // DispatchResult
#include "status/status_ring.hpp"  // kMaxRingPixels

class Adafruit_NeoPixel;
class AuroraPaletteClient;

namespace wisp {

class PaintDistributor;
class WispConfig;
class ZoneSelector;
class StatusEmitter;
class ArtnetEmitter;
enum class WispSourceMode : uint8_t;

class WispController {
 public:
  WispController(CurrentPalette& palette, PaintDistributor& paint,
                 WispConfig& config, ZoneSelector& zones,
                 AuroraPaletteClient& aurora, StatusEmitter& status,
                 ArtnetEmitter& artnet, Adafruit_NeoPixel& strip)
      : palette_(palette),
        paint_(paint),
        config_(config),
        zones_(zones),
        aurora_(aurora),
        status_(status),
        artnet_(artnet),
        strip_(strip) {}

  // Idempotent: safe at boot (from NVS) and on every mode flip.
  void applySourceModeTransition(WispSourceMode mode);

  // Aurora active-palette callback. Zone latch + filter + palette push + ring.
  void onAuroraPalette(int zone, const Palette& p);

  // Apply the DispatchResult from a CONTROL_OP. MeshRouter result callback.
  void applyOpResult(DispatchResult res);

  // Loop-task edge detector: Aurora stream drop holds paint off + re-emits.
  void tickAuroraLiveness();

  // OSC palette input (flat RGB floats 0..1). Applied only in Osc mode;
  // an unchanged palette only refreshes liveness. Never turns paint on.
  void onOscPalette(const float* rgb, size_t colours, const char* via,
                    uint32_t nowMs);

  // Osc mode with no palette for kOscStaleMs: clear + paint off, like an
  // Aurora stream drop.
  void tickOscLiveness(uint32_t now);

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
  ZoneSelector& zones_;
  AuroraPaletteClient& aurora_;
  StatusEmitter& status_;
  ArtnetEmitter& artnet_;
  Adafruit_NeoPixel& strip_;

  bool auroraWasStreaming_ = false;

  static constexpr uint32_t kOscStaleMs = 10000;
  static constexpr size_t kOscMaxFloats = 150;  // kManualPaletteMaxColors * 3
  float oscLast_[kOscMaxFloats] = {0};
  size_t oscLastFloats_ = 0;  // 0 = no live OSC palette
  uint32_t oscLastRxMs_ = 0;
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
