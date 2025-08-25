#include "./wifi.hpp"

#include <AsyncTCP.h>
#include <DNSServer.h>
#include <WiFi.h>

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
std::vector<Color> artnetData = {Color(0), Color(0)};
unsigned long lastDmxFrameMs;
uint8_t seq = 0;
bool newDmxData = false;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence,
                uint8_t *data) {
  lastDmxFrameMs = millis();
#ifdef LAMP_DEBUG
  if (universe == 1 && sequence != (seq + 1)) {
    Serial.printf("dmx frame skipped seq: %d - prev seq: %d\n", sequence, seq);
  }
#endif
  if (universe == 1) {
    seq = sequence;
    artnetData = {Color(data[0], data[1], data[2], data[3]),
                  Color(data[4], data[5], data[6], data[7])};
  }
  newDmxData = true;
}

class CaptiveRequestHandler : public AsyncWebHandler {
 public:
  bool canHandle(__unused AsyncWebServerRequest *request) const override {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/configurator.html", String(), false);
  }
};

void WifiComponent::begin(std::string name) {
#ifdef LAMP_DEBUG
  Serial.printf("Starting Wifi Async Client\n");
#endif
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(SECRET_COORDINATOR_SSID, SECRET_COORDINATOR_SHARED_PASS);
  artnet.setArtDmxCallback(onDmxFrame);
  artnet.begin();

  WiFi.softAP(name.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler())
      .setFilter(ON_AP_FILTER);  // only when requested from AP
  server.begin();
};

void WifiComponent::tick() {
  dnsServer.processNextRequest();
  // artnet.read();
}

bool WifiComponent::hasArtnetData() { return newDmxData; }

std::vector<Color> WifiComponent::getArtnetData() {
  newDmxData = false;
  return artnetData;
};

unsigned long WifiComponent::getLastArtnetFrameTimeMs() {
  return lastDmxFrameMs;
}
}  // namespace lamp