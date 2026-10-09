#include "status/status_emitter.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <lampos/led_types.hpp>

#include "paint/current_palette.hpp"
#include "net/mesh_link.hpp"
#include "net/wifi_link.hpp"
#include "config/wisp_config.hpp"
#include "core/wisp_controller.hpp"
#include "wire/lamp_protocol.hpp"
#include "status/status_json.hpp"
#include "status/seq_source.hpp"

namespace wisp {

namespace {

constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr size_t kPaletteIdPrefixLen = lamp_protocol::WISP_HELLO_PALETTE_ID_PREFIX_LEN;

}  // namespace

void StatusEmitter::begin(MeshLink* mesh, const WispController* controller,
                          WispConfig* config, CurrentPalette* palette,
                          SeqSource* seq, WifiLink* wifi) {
  mesh_ = mesh;
  controller_ = controller;
  config_ = config;
  palette_ = palette;
  seq_ = seq;
  wifi_ = wifi;
}

void StatusEmitter::startTimer() {
  if (!statusTimer_) {
    statusTimer_ = xTimerCreate(
        "wisp_status",
        pdMS_TO_TICKS(kStatusIntervalMs),
        pdTRUE,  // auto-reload
        this,
        [](TimerHandle_t t) {
          auto* self = static_cast<StatusEmitter*>(pvTimerGetTimerID(t));
          if (self) self->statusDue_.store(true, std::memory_order_relaxed);
        });
    if (statusTimer_) {
      xTimerStart(statusTimer_, 0);
    } else {
      Serial.println("[wisp.beacon] xTimerCreate(status) failed");
    }
  }
}

void StatusEmitter::triggerOnChange() {
  statusDue_.store(true, std::memory_order_relaxed);
  if (statusTimer_) {
    xTimerReset(statusTimer_, 0);
  }
  burstUntilMs_ = millis() + kStatusBurstMs;
}

void StatusEmitter::pump() {
  checkConnEdges();
  const uint32_t now = millis();
  bool bursting = false;
  if (burstUntilMs_ != 0) {
    if (static_cast<int32_t>(now - burstUntilMs_) < 0) {
      bursting = true;
    } else {
      burstUntilMs_ = 0;
    }
  }
  if (!statusDue_.load(std::memory_order_relaxed) && !bursting) return;
  const uint32_t minInterval = bursting ? kStatusBurstIntervalMs : kMinEmitIntervalMs;
  if (haveEmitted_ && now - lastEmitMs_ < minInterval) return;
  statusDue_.store(false, std::memory_order_relaxed);
  haveEmitted_ = true;
  lastEmitMs_ = now;
  emitStatus();
}

void StatusEmitter::checkConnEdges() {
  const bool wifiConn = WiFi.isConnected();
  if (!haveLastConnState_) {
    haveLastConnState_ = true;
  } else if (wifiConn != lastWifiConnected_) {
    Serial.printf("[wisp.beacon] connectivity edge wifi=%d\n", (int)wifiConn);
    triggerOnChange();
  }
  lastWifiConnected_ = wifiConn;
}

void StatusEmitter::emitStatus() {
  if (!mesh_) return;

#ifdef LAMP_DEBUG
  Serial.printf("[wispheap] free=%u largest=%u\n",
                (unsigned)esp_get_free_heap_size(),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#endif

  uint8_t srcMac[6] = {0};
  mesh_->getMac(srcMac);

  const bool wifiConn   = WiFi.isConnected();
  const bool sourceLive = controller_ && controller_->sourceLive();
  const bool wifiChannelMismatch = wifi_ && wifi_->channelMismatch();
  const int  wifiApChannel = wifi_ ? wifi_->apChannel() : 0;

  char paletteIdPrefix[kPaletteIdPrefixLen + 1] = {0};
  if (palette_) {
    const size_t n = palette_->copyPaletteIdPrefix(paletteIdPrefix,
                                                   kPaletteIdPrefixLen);
    paletteIdPrefix[n] = '\0';
  }

  const uint32_t lastSeenMs = millis();

  // manualPalette excluded: emitted separately via MSG_WISP_PALETTE (buildWispPalette).
  const char* sourceName = "off";
  const char* oscRole = nullptr;
  if (config_) {
    switch (config_->sourceMode()) {
      case WispSourceMode::Off:    sourceName = "off";    break;
      case WispSourceMode::Manual: sourceName = "manual"; break;
      case WispSourceMode::Osc:    sourceName = "osc";    break;
    }
    oscRole = oscRoleName(config_->oscRole());
  }

  uint8_t offR = 0, offG = 0, offB = 0, offW = 0;
  bool hasOffColor = false;
  uint8_t shuffleSeed = 0;
  uint32_t driftIntervalMs = 0;
  uint8_t driftFadePct = 0;
  const char* wispName = "";
  bool hasPassword = false;
  const char* ledType = "GRB";
  uint16_t pixelCount = 30;
  uint32_t opSeq = 0;
  uint8_t brightness = 100;
  if (config_) {
    const auto off = config_->offColor();
    offR = off.r; offG = off.g; offB = off.b; offW = off.w;
    hasOffColor = true;
    shuffleSeed = config_->shuffleSeed();
    driftIntervalMs = config_->driftIntervalMs();
    driftFadePct = config_->driftFadePct();
    wispName = config_->name().c_str();
    hasPassword = config_->password().length() > 0;
    ledType = lampos::led::byteOrderToString(config_->ledFormat());
    pixelCount = config_->pixelCount();
    opSeq = config_->opSeq();
    brightness = config_->brightness();
  }
  const WispStatusFields fields{
      wifiConn, sourceLive, paletteIdPrefix, lastSeenMs,
      sourceName, oscRole, offR, offG, offB, offW, hasOffColor, shuffleSeed,
      driftIntervalMs, driftFadePct, wispName, hasPassword,
      ledType, pixelCount, opSeq, brightness,
      wifiChannelMismatch, wifiApChannel };

  char jsonBuf[kStatusJsonBufLen];
  const size_t jsonLen = buildWispStatusJson(
      fields, jsonBuf, sizeof(jsonBuf), lamp_protocol::CONTROL_MAX_PAYLOAD);

  // Emit palette first: a wispStatus build failure must not suppress it.
  emitPalette();

  if (jsonLen == 0) {
    Serial.println("[wisp.beacon] wispStatus JSON build failed");
    return;
  }

  // SeqSource mux: concurrent HELLO and wispStatus emits must not share a seq.
  uint8_t frame[lamp_protocol::CONTROL_MAX_SIZE];
  size_t frameLen = 0;
  uint16_t seq = 0;
  WISP_SEQ_PORTMUX_ENTER(&seq_->mux);
  seq = seq_->next();
  frameLen = lamp_protocol::buildControlOp(
      frame, sizeof(frame), seq,
      kBroadcastMac, srcMac,
      reinterpret_cast<const uint8_t*>(jsonBuf), jsonLen);
  WISP_SEQ_PORTMUX_EXIT(&seq_->mux);
  if (!frameLen) {
    Serial.println("[wisp.beacon] buildControlOp(wispStatus) failed");
    return;
  }
  mesh_->broadcast(frame, frameLen);
}

void StatusEmitter::emitPalette() {
  if (!mesh_) return;

  uint8_t srcMac[6] = {0};
  mesh_->getMac(srcMac);

  uint8_t rgb[lamp_protocol::kMaxWispPaletteColors * 3];
  uint8_t w[lamp_protocol::kMaxWispPaletteColors];
  size_t count = 0;
  if (config_) {
    count = config_->copyManualPalette(rgb, w,
                                       lamp_protocol::kMaxWispPaletteColors);
  }

  uint8_t frame[lamp_protocol::WISP_PALETTE_MAX_SIZE];
  size_t frameLen = 0;
  uint16_t seq = 0;
  WISP_SEQ_PORTMUX_ENTER(&seq_->mux);
  seq = seq_->next();
  frameLen = lamp_protocol::buildWispPalette(
      frame, sizeof(frame), seq, srcMac,
      count > 0 ? rgb : nullptr, count > 0 ? w : nullptr, count);
  WISP_SEQ_PORTMUX_EXIT(&seq_->mux);
  if (!frameLen) {
    Serial.println("[wisp.beacon] buildWispPalette failed");
    return;
  }
  mesh_->broadcast(frame, frameLen);
}

}  // namespace wisp
