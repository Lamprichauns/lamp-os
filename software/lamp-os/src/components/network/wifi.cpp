#include "./wifi.hpp"

#include <AsyncTCP.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include "../../secrets.hpp"
#include "../../util/color.hpp"
#include "./artnet.hpp"
#include "./wifi.hpp"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

static DNSServer dnsServer;
static AsyncWebServer server(80);

namespace lamp {
DNSServer dnsServer;
WiFiUDP UdpSend;
ArtnetWifi artnet;
uint32_t lastWifiScanMs = 0;
StaState staStatus = DISCONNECTED;
std::vector<Color> artnetData = {Color(0), Color(0)};
unsigned long lastDmxFrameMs;
uint8_t seq = 0;
bool newDmxData = false;
wl_status_t previousStatus = WL_DISCONNECTED;

void onWiFiEvent(WiFiEvent_t event) {
#ifdef LAMP_DEBUG
  Serial.printf("**** WIFI Event %d\n", event);
#endif
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.begin();
  }
};

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence,
                uint8_t *data) {
  lastDmxFrameMs = millis();
  if (universe == 1) {
    artnetData = {Color(data[0], data[1], data[2], data[3]),
                  Color(data[4], data[5], data[6], data[7])};
    newDmxData = true;
#ifdef LAMP_DEBUG
    if (sequence != 1 && sequence != (seq + 1)) {
      Serial.printf("dmx frame skipped seq: %d - prev seq: %d\n", sequence,
                    seq);
    }
#endif
    seq = sequence;
  }
};

void scanForRouter() {
  if (staStatus == DISCONNECTED &&
      millis() > lastWifiScanMs + ARTNET_NETWORK_SCAN_MS) {
#ifdef LAMP_DEBUG
    Serial.println("*****Connecting to artnet enabled Wifi router");
#endif
    WiFi.mode(WIFI_AP_STA);
    if (WiFi.begin(SECRET_COORDINATOR_SSID, SECRET_COORDINATOR_SHARED_PASS,
                   7) == WL_CONNECTED) {
#ifdef LAMP_DEBUG
      Serial.println("****Wifi connection to router successful");
#endif
      staStatus = CONNECTED;
    }
    lastWifiScanMs = millis();
  }
};

class CaptiveRequestHandler : public AsyncWebHandler {
 public:
  bool canHandle(__unused AsyncWebServerRequest *request) const override {
    return true;
  };

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/configurator.html", String(), false);
  };
};

void WifiComponent::begin(std::string name) {
#ifdef LAMP_DEBUG
  Serial.printf("Starting Wifi Async Client\n");
#endif
  WiFi.mode(WIFI_AP);
  WiFi.onEvent(onWiFiEvent);
  WiFi.disconnect();
  WiFi.softAP(name.c_str(), emptyString, 7);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler())
      .setFilter(ON_AP_FILTER);  // only when requested from AP
  server.begin();
  artnet.setArtDmxCallback(onDmxFrame);
  artnet.begin();
};

void WifiComponent::tick() {
  dnsServer.processNextRequest();
  //scanForRouter();
};

bool WifiComponent::hasArtnetData() { return newDmxData; }

std::vector<Color> WifiComponent::getArtnetData() {
  newDmxData = false;
  return artnetData;
};

unsigned long WifiComponent::getLastArtnetFrameTimeMs() {
  return lastDmxFrameMs;
};
}  // namespace lamp