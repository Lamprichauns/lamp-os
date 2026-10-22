// WifiLink hosts the wisp's softAP for pre-mesh lamps. Brought up once at
// boot on the mesh channel and left up; the ArtNet emitter gates on
// canBroadcast() and pins the egress netif off isAp(). It also joins the
// stored STA network while staWanted() holds, on the policy in StaJoin.
//
// The mesh pins the radio to LAMP_ESPNOW_CHANNEL and sends ESP-NOW on
// WIFI_IF_STA. The softAP runs in WIFI_AP_STA on the same channel, so the STA
// netif (and the mesh) stays up and same-channel WiFi doesn't disturb ESP-NOW.

#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include <atomic>
#include <functional>
#include <string>

#include "net/sta_join.hpp"

namespace wisp {

class WispConfig;

class WifiLink {
 public:
  enum class Mode : uint8_t { Off, Sta, Ap };

  // Bind to WispConfig. Joins nothing: the STA join runs from loop() only
  // while it is wanted. Idempotent. config must outlive this WifiLink
  // (main.cpp owns both).
  void begin(WispConfig* config);

  // Re-read creds and restart the join schedule: drops any association and,
  // while the join is wanted, attempts at once. Called after a setWifi op
  // persists new creds. Clears the channel-mismatch guard, which is
  // otherwise sticky: after a mismatch the wisp does not retry until this or
  // until the join becomes wanted again (re-entering Osc, or Ethernet going
  // down in Osc).
  void reconnect();

  // Loop-task driver. `wanted` is staWanted(): Osc, with Ethernet not up.
  // While wanted and creds exist it scans the mesh channel for the stored
  // SSID and joins only an AP found there, retrying on StaJoin's backoff (no
  // core auto-reconnect). Unwanted drops the association and stops retrying;
  // creds stay in NVS. Becoming wanted again clears a channel-mismatch halt.
  // Also applies a deferred STA_GOT_IP channel check: the event only records
  // the channel, the radio work runs here off the WiFi event task.
  void loop(bool wanted, uint32_t nowMs);

  // softAP role on the mesh channel (WIFI_AP_STA so ESP-NOW keeps working).
  // ssid/pass are the wisp's own network, not the WispConfig STA creds.
  // Idempotent: a no-op when the AP is already up.
  void startSoftAp(const char* ssid, const char* pass);

  // True when ArtNet frames can egress: STA associated, or softAP up.
  bool canBroadcast() const;
  // A UDP send left to default routing in WIFI_AP_STA can egress the wrong
  // netif, so ArtNet pins the interface per role off this.
  bool isAp() const { return mode_ == Mode::Ap; }

  // Fill out[] with the IPs of stations currently joined to the softAP (DHCP
  // leases), returning the count. Zero outside AP mode. ArtNet unicasts to each
  // because WiFi broadcast is unreliable to sleepy lamps.
  size_t apClientIps(IPAddress* out, size_t maxOut) const;

  // Snapshot accessors. Cheap. Callable from any task.
  bool isConnected() const;

  // True once an AP association landed on a channel other than the mesh
  // channel and the guard dropped it. Cleared by reconnect() or by the join
  // becoming wanted again. apChannel() is the offending channel (0 when no
  // mismatch). Loop-task state, read by the status emitter.
  bool channelMismatch() const { return channelMismatch_; }
  int apChannel() const { return lastStaChannel_; }

  // Fired from loop() only when channelMismatch() flips, so the status emitter
  // can re-emit at once instead of waiting for the next heartbeat.
  void setOnChangeCallback(std::function<void()> cb) { onChange_ = std::move(cb); }

  // SSID/password reflected from the bound WispConfig. Returns empty
  // strings before begin() is called or when the store has no creds.
  std::string ssid() const;
  std::string password() const;

 private:
  WispConfig* config_ = nullptr;
  bool started_ = false;
  Mode mode_ = Mode::Off;
  bool apUp_ = false;

  bool channelMismatch_ = false;
  int lastStaChannel_ = 0;

  void apply(StaAction action);
  void startScan();
  void pollScan(uint32_t nowMs);
  void dropJoin();
  void checkChannel(int ch);
  void clearMismatch();
  void onStaDisconnected(uint32_t nowMs);
  void onStaConnected();

  StaJoin join_;
  bool wanted_ = false;
  uint8_t scanBssid_[6] = {0};
  // Written by the WiFi event task, read by loop(). Every event takes the
  // next eventSeq_; lastDisconnectSeq_ is the latest STA_DISCONNECTED the
  // wisp did not cause (reason other than ASSOC_LEAVE), lastConnectSeq_ the
  // latest GOT_IP and lastConnectChannel_ its channel. loop() applies new
  // events in sequence order; appliedSeq_ is the newest applied, and anything
  // at or below it is stale. dropJoin() swallows every event so far, so a
  // GOT_IP for a link already dropped is never adopted.
  std::atomic<uint32_t> eventSeq_{0};
  std::atomic<uint32_t> lastDisconnectSeq_{0};
  std::atomic<uint32_t> lastConnectSeq_{0};
  std::atomic<int> lastConnectChannel_{0};
  uint32_t appliedSeq_ = 0;

  std::function<void()> onChange_;
};

}  // namespace wisp
