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

void onWiFiEvent(WiFiEvent_t event) {
#ifdef LAMP_DEBUG
  Serial.printf("WIFI Event %d\n", event);
#endif
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

WifiComponent::WifiComponent() {};

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
  artnet.begin();
};

void WifiComponent::tick() { dnsServer.processNextRequest(); };

bool WifiComponent::hasArtnetData() { return artnet.newDmxData; }

std::vector<Color> WifiComponent::getArtnetData() {
  artnet.newDmxData = false;
  return artnet.artnetData;
};

unsigned long WifiComponent::getLastArtnetFrameTimeMs() {
  return artnet.lastDmxFrameMs;
};
}  // namespace lamp