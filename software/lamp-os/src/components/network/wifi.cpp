#include "./wifi.hpp"

#include <AsyncTCP.h>
#include <DNSServer.h>
#include <WiFi.h>

#include "../../secrets.hpp"
#include "../../util/color.hpp"
#include "./wifi.hpp"
#include "ArtnetWifi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

namespace lamp {
DNSServer dnsServer;
AsyncWebServer server(80);
WiFiUDP UdpSend;
ArtnetWifi artnet;
std::vector<Color> artnetData = {Color(0), Color(0)};
unsigned long lastDmxFrameMs;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence,
                uint8_t *data) {
  lastDmxFrameMs = millis();
  if (universe == 1) {
    artnetData = {Color(data[1], data[2], data[3], data[4]),
                  Color(data[5], data[6], data[7], data[8])};
  }
}

class CaptiveRequestHandler : public AsyncWebHandler {
 public:
  CaptiveRequestHandler() {};
  virtual ~CaptiveRequestHandler() {};

  bool canHandle(AsyncWebServerRequest *request) { return true; };

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/configurator.html", String(), false);
  };
};

void WifiComponent::begin(std::__cxx11::string name) {
  WiFi.begin(SECRET_COORDINATOR_SSID, SECRET_COORDINATOR_SHARED_PASS);
  artnet.setArtDmxCallback(onDmxFrame);
  artnet.begin();

  WiFi.softAP(name.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
};

void WifiComponent::tick() {
  dnsServer.processNextRequest();
  artnet.read();
}

std::vector<Color> WifiComponent::getArtnetData() { return artnetData; };

unsigned long WifiComponent::getLastArtnetFrameTimeMs() {
  return lastDmxFrameMs;
}
}  // namespace lamp