#include "net/wifi_link.hpp"

#include <WiFi.h>
#include <esp_wifi.h>

#include "config/wisp_config.hpp"
#include "net/mesh_link.hpp"  // LAMP_ESPNOW_CHANNEL

namespace wisp {

namespace {
// The capture-less WiFi.onEvent lambda runs on the event task; it reaches the
// bound WifiLink through this. begin() is idempotent, so one instance.
WifiLink* s_self = nullptr;

// Mesh-channel scan dwell. One channel, so the radio is off the mesh's
// listen for at most this long per attempt.
constexpr uint32_t kScanDwellMs = 120;
}  // namespace

void WifiLink::begin(WispConfig* config) {
  if (started_) return;
  started_ = true;
  config_ = config;
  mode_ = Mode::Sta;
  s_self = this;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  // Retries are StaJoin's, on a backoff. The core's auto-reconnect would
  // rescan every channel for a missing AP every few seconds, each scan pulling
  // the one radio off the mesh long enough for lamps to age out.
  WiFi.setAutoReconnect(false);

  WiFi.onEvent([](WiFiEvent_t event, [[maybe_unused]] WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.printf("[wifi] got IP: %s\n",
                    WiFi.localIP().toString().c_str());
      // One shared radio: STA follows the AP's channel, dragging ESP-NOW with
      // it. Hand the channel to loop(); the radio fix runs off the event task.
      if (s_self) {
        s_self->lastConnectChannel_.store(WiFi.channel(),
                                          std::memory_order_relaxed);
        s_self->lastConnectSeq_.store(
            s_self->eventSeq_.fetch_add(1, std::memory_order_relaxed) + 1,
            std::memory_order_relaxed);
      }
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      const uint8_t reason = info.wifi_sta_disconnected.reason;
      Serial.printf("[wifi] disconnected reason=%u\n", (unsigned)reason);
      // ASSOC_LEAVE is the wisp's own disconnect; it fails no attempt.
      if (s_self && reason != WIFI_REASON_ASSOC_LEAVE) {
        s_self->lastDisconnectSeq_.store(
            s_self->eventSeq_.fetch_add(1, std::memory_order_relaxed) + 1,
            std::memory_order_relaxed);
      }
#ifdef LAMP_DEBUG
    } else if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
      const uint8_t* m = info.wifi_ap_staconnected.mac;
      Serial.printf("[wifi] AP station joined %02X:%02X:%02X:%02X:%02X:%02X\n",
                    m[0], m[1], m[2], m[3], m[4], m[5]);
    } else if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
      Serial.printf("[wifi] AP station got IP %s\n",
                    IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
    } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
      Serial.println("[wifi] AP station left");
#endif
    }
  });
}

void WifiLink::loop(bool wanted, uint32_t nowMs) {
  if (wanted != wanted_) {
    if (wanted) {
      clearMismatch();
    } else {
      Serial.println("[wifi] source does not use the network; STA idle");
    }
  }
  wanted_ = wanted;
  apply(join_.setWanted(wanted, nowMs));

  const uint32_t conn = lastConnectSeq_.load(std::memory_order_relaxed);
  const uint32_t disc = lastDisconnectSeq_.load(std::memory_order_relaxed);
  const bool newConn = static_cast<int32_t>(conn - appliedSeq_) > 0;
  const bool newDisc = static_cast<int32_t>(disc - appliedSeq_) > 0;
  if (newConn && (!newDisc || static_cast<int32_t>(conn - disc) < 0)) {
    appliedSeq_ = conn;
    onStaConnected();
    if (newDisc && static_cast<int32_t>(disc - appliedSeq_) > 0) {
      appliedSeq_ = disc;
      onStaDisconnected(nowMs);
    }
  } else if (newDisc) {
    appliedSeq_ = disc;
    onStaDisconnected(nowMs);
    if (newConn && static_cast<int32_t>(conn - appliedSeq_) > 0) {
      appliedSeq_ = conn;
      onStaConnected();
    }
  }
  // Backstop for a link that went down without an event reaching here: the
  // netif CONNECTED bit, unlike WiFi.status(), is not latched.
  if (join_.state() == StaJoin::State::Linked && !WiFi.STA.connected()) {
    onStaDisconnected(nowMs);
  }

  pollScan(nowMs);
  const StaJoin::State before = join_.state();
  const bool hasCreds = config_ && config_->wifiSsid().length() > 0 &&
                        config_->wifiPw().length() > 0;
  apply(join_.step(nowMs, hasCreds));
  if ((before == StaJoin::State::Joining ||
       before == StaJoin::State::Scanning) &&
      join_.state() == StaJoin::State::Waiting) {
    Serial.printf("[wifi] %s %s timed out; retry in %lus\n", ssid().c_str(),
                  before == StaJoin::State::Joining ? "join" : "scan",
                  (unsigned long)(join_.lastWaitMs() / 1000));
  }
}

void WifiLink::onStaDisconnected(uint32_t nowMs) {
  const StaJoin::State was = join_.state();
  join_.onDisconnected(nowMs);
  if (was == StaJoin::State::Joining || was == StaJoin::State::Linked) {
    Serial.printf("[wifi] %s %s; retry in %lus\n", ssid().c_str(),
                  was == StaJoin::State::Linked ? "link lost" : "join failed",
                  (unsigned long)(join_.lastWaitMs() / 1000));
  }
}

void WifiLink::onStaConnected() {
  const StaAction a = join_.onConnected();
  apply(a);
  if (a == StaAction::None) {
    checkChannel(lastConnectChannel_.load(std::memory_order_relaxed));
  }
}

void WifiLink::clearMismatch() {
  lastStaChannel_ = 0;
  if (!channelMismatch_) return;
  channelMismatch_ = false;
  if (onChange_) onChange_();
}

void WifiLink::apply(StaAction action) {
  switch (action) {
    case StaAction::None:
      break;
    case StaAction::StartScan:
      startScan();
      break;
    case StaAction::Begin: {
      const std::string s = ssid();
      const std::string p = password();
      Serial.printf("[wifi] %s found on ch %d; joining\n", s.c_str(),
                    LAMP_ESPNOW_CHANNEL);
      WiFi.begin(s.c_str(), p.c_str(), LAMP_ESPNOW_CHANNEL, scanBssid_);
      break;
    }
    case StaAction::Drop:
      dropJoin();
      break;
  }
}

void WifiLink::startScan() {
  const std::string s = ssid();
  // Async and filtered to the stored SSID on the mesh channel; pollScan()
  // reads the result.
  const int16_t r = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/false,
                                      /*passive=*/false, kScanDwellMs,
                                      LAMP_ESPNOW_CHANNEL, s.c_str());
  if (r == WIFI_SCAN_FAILED) {
    Serial.println("[wifi] mesh-channel scan failed to start");
  }
}

void WifiLink::pollScan(uint32_t nowMs) {
  if (join_.state() != StaJoin::State::Scanning) return;
  const int16_t n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  int best = -1;
  for (int i = 0; i < n; ++i) {
    if (WiFi.channel(i) != LAMP_ESPNOW_CHANNEL) continue;
    if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
  }
  const bool found = best >= 0;
  if (found) WiFi.BSSID(static_cast<uint8_t>(best), scanBssid_);
  WiFi.scanDelete();
  const StaAction a = join_.onScanDone(found, nowMs);
  if (n == WIFI_SCAN_FAILED) {
    Serial.printf("[wifi] mesh-channel scan failed; retry in %lus\n",
                  (unsigned long)(join_.lastWaitMs() / 1000));
  } else if (!found) {
    Serial.printf("[wifi] %s not on ch %d; retry in %lus\n", ssid().c_str(),
                  LAMP_ESPNOW_CHANNEL,
                  (unsigned long)(join_.lastWaitMs() / 1000));
  }
  apply(a);
}

void WifiLink::dropJoin() {
  esp_wifi_scan_stop();
  WiFi.scanDelete();
  // Drop the association but keep the stored creds and the radio up; the STA
  // interface must stay up for ESP-NOW.
  WiFi.disconnect(/*wifioff=*/false, /*eraseap=*/false);
  appliedSeq_ = eventSeq_.load(std::memory_order_relaxed);
  // Setting the channel is refused while scanning, connecting or serving
  // softAP stations, so only when the radio has actually left the mesh.
  uint8_t primary = 0;
  wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&primary, &second) == ESP_OK &&
      primary != LAMP_ESPNOW_CHANNEL) {
    esp_wifi_set_channel(LAMP_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  }
}

void WifiLink::checkChannel(int ch) {
  if (ch == 0) return;
  const bool wasMismatch = channelMismatch_;
  if (ch == LAMP_ESPNOW_CHANNEL) {
    channelMismatch_ = false;
    lastStaChannel_ = 0;
    if (wasMismatch && onChange_) onChange_();
    return;
  }
  channelMismatch_ = true;
  lastStaChannel_ = ch;
  Serial.printf("[wifi] AP ch=%d != mesh ch=%d; dropping association, "
                "re-pinning radio to mesh, retries halted\n",
                ch, LAMP_ESPNOW_CHANNEL);
  // Shed the wrong-channel association and snap the radio back so ESP-NOW is
  // restored; retries halt so nothing re-drags the radio.
  join_.halt();
  dropJoin();
  if (!wasMismatch && onChange_) onChange_();
}

void WifiLink::reconnect() {
  if (!config_) {
    Serial.println("[wifi] reconnect called before begin(); ignoring");
    return;
  }
  clearMismatch();
  if (join_.restart(millis()) == StaAction::Drop) dropJoin();
  if (ssid().empty() || password().empty()) {
    Serial.println(
        "[wifi] no creds; staying disconnected (radio stays up for ESP-NOW)");
  } else if (!wanted_) {
    Serial.printf("[wifi] creds for ssid=%s stored; joins only in Osc mode\n",
                  ssid().c_str());
  } else {
    Serial.printf("[wifi] reconnect requested ssid=%s\n", ssid().c_str());
  }
}

void WifiLink::startSoftAp(const char* ssid, const char* pass) {
  if (mode_ == Mode::Ap && apUp_) return;
  WiFi.mode(WIFI_AP_STA);
  apUp_ = WiFi.softAP(ssid, pass, LAMP_ESPNOW_CHANNEL);
  mode_ = Mode::Ap;
  if (apUp_) {
    Serial.printf("[wifi] softAP up ssid=%s ch=%d ip=%s\n", ssid,
                  LAMP_ESPNOW_CHANNEL, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("[wifi] softAP start FAILED ssid=%s\n", ssid);
  }
}

bool WifiLink::canBroadcast() const {
  if (mode_ == Mode::Ap) return apUp_;
  if (mode_ == Mode::Sta) return WiFi.status() == WL_CONNECTED;
  return false;
}

size_t WifiLink::apClientIps(IPAddress* out, size_t maxOut) const {
  if (mode_ != Mode::Ap) return 0;
  // ponytail: softAP DHCP leases run sequentially from .2, so derive client
  // IPs from the station count. A lease gap after disconnect/reconnect churn
  // would misaddress a lamp; swap for a real lease enumeration if that bites.
  const uint8_t num = WiFi.softAPgetStationNum();
  const IPAddress base = WiFi.softAPIP();
  size_t count = 0;
  for (uint8_t i = 0; i < num && count < maxOut; ++i) {
    out[count++] = IPAddress(base[0], base[1], base[2], 2 + i);
  }
  return count;
}

bool WifiLink::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

std::string WifiLink::ssid() const {
  if (!config_) return {};
  return std::string(config_->wifiSsid().c_str());
}

std::string WifiLink::password() const {
  if (!config_) return {};
  return std::string(config_->wifiPw().c_str());
}

}  // namespace wisp
