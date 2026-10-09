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
}  // namespace

void WifiLink::begin(WispConfig* config) {
  if (started_) return;
  started_ = true;
  config_ = config;
  mode_ = Mode::Sta;
  s_self = this;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  // Retries are loop()'s, on a backoff. The core's auto-reconnect would rescan
  // for a missing AP every few seconds, each scan pulling the one radio off
  // the mesh channel long enough for lamps to age out.
  WiFi.setAutoReconnect(false);

  WiFi.onEvent([](WiFiEvent_t event, [[maybe_unused]] WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.printf("[wifi] got IP: %s\n",
                    WiFi.localIP().toString().c_str());
      // One shared radio: STA follows the AP's channel, dragging ESP-NOW with
      // it. Hand the channel to loop(); the radio fix runs off the event task.
      if (s_self) {
        s_self->pendingStaChannel_.store(WiFi.channel(),
                                         std::memory_order_relaxed);
      }
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.println("[wifi] disconnected");
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
  checkChannel();
  if (wanted != wanted_) {
    wanted_ = wanted;
    if (wanted) {
      backoff_.reset(nowMs);
    } else {
      backoff_.stop();
      dropJoin();
      Serial.println("[wifi] source does not use the network; STA idle");
    }
  }
  if (!wanted_ || channelMismatch_) return;

  const wl_status_t st = WiFi.status();
  if (joining_) {
    if (st == WL_CONNECTED) {
      joining_ = false;
      linked_ = true;
      backoff_.stop();
      return;
    }
    const bool noAp = st == WL_NO_SSID_AVAIL;
    if (noAp || st == WL_CONNECT_FAILED ||
        nowMs - joinStartMs_ >= kJoinTimeoutMs) {
      dropJoin();
      const uint32_t waitMs = backoff_.failed(nowMs);
      Serial.printf("[wifi] join %s failed (%s); retry in %lus\n",
                    ssid().c_str(), noAp ? "AP not found" : "no link",
                    (unsigned long)(waitMs / 1000));
    }
    return;
  }
  if (linked_ && st != WL_CONNECTED) {
    linked_ = false;
    backoff_.reset(nowMs);
    backoff_.failed(nowMs);
    Serial.println("[wifi] link lost; retrying on backoff");
    return;
  }
  if (!linked_ && backoff_.due(nowMs)) beginJoin(nowMs);
}

void WifiLink::beginJoin(uint32_t nowMs) {
  const std::string s = ssid();
  const std::string p = password();
  if (s.empty() || p.empty()) {
    backoff_.stop();
    return;
  }
  Serial.printf("[wifi] joining ssid=%s on ch %d\n", s.c_str(),
                LAMP_ESPNOW_CHANNEL);
  // Pinning the channel scans only the mesh channel, so the radio never
  // leaves it and an AP on another channel is never joined.
  WiFi.begin(s.c_str(), p.c_str(), LAMP_ESPNOW_CHANNEL);
  joining_ = true;
  joinStartMs_ = nowMs;
}

void WifiLink::dropJoin() {
  joining_ = false;
  linked_ = false;
  // (false, false): drop association, keep stored creds and radio up; the STA
  // interface must stay up for ESP-NOW.
  WiFi.disconnect(false, false);
  esp_wifi_set_channel(LAMP_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
}

void WifiLink::checkChannel() {
  const int ch = pendingStaChannel_.exchange(0, std::memory_order_relaxed);
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
                "re-pinning radio to mesh, auto-reconnect off\n",
                ch, LAMP_ESPNOW_CHANNEL);
  // Shed the wrong-channel association and snap the radio back so ESP-NOW is
  // restored; the backoff stops so nothing re-drags the radio.
  dropJoin();
  backoff_.stop();
  if (!wasMismatch && onChange_) onChange_();
}

void WifiLink::reconnect() {
  if (!config_) {
    Serial.println("[wifi] reconnect called before begin(); ignoring");
    return;
  }
  channelMismatch_ = false;
  lastStaChannel_ = 0;
  pendingStaChannel_.store(0, std::memory_order_relaxed);
  dropJoin();
  if (ssid().empty() || password().empty()) {
    backoff_.stop();
    Serial.println(
        "[wifi] no creds; staying disconnected (radio stays up for ESP-NOW)");
    return;
  }
  if (!wanted_) {
    backoff_.stop();
    Serial.printf("[wifi] creds for ssid=%s stored; joins only in Osc mode\n",
                  ssid().c_str());
    return;
  }
  backoff_.reset(millis());
  Serial.printf("[wifi] reconnect requested ssid=%s\n", ssid().c_str());
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
