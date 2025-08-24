#include "./wifi.hpp"

#include <DNSServer.h>
#include <WiFi.h>

#include "../../secrets.hpp"
#include "../../util/color.hpp"
#include "./wifi.hpp"
#include "ArtnetWifi.h"
#include "SPIFFS.h"

namespace lamp {
DNSServer dnsServer;
WiFiUDP UdpSend;
ArtnetWifi artnet;
std::vector<Color> artnetData = {Color(0), Color(0)};
unsigned long lastDmxFrameMs;

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence,
                uint8_t *data) {
  lastDmxFrameMs = millis();
  if (universe == 1) {
    artnetData = {Color(data[0], data[1], data[2], data[3]),
                  Color(data[4], data[5], data[6], data[7])};
  }
}

void WifiComponent::begin(std::string name) {
#ifdef LAMP_DEBUG
  Serial.printf("Starting Wifi Async Client\n");
#endif
  WiFi.begin(SECRET_COORDINATOR_SSID, SECRET_COORDINATOR_SHARED_PASS);
  artnet.setArtDmxCallback(onDmxFrame);
  artnet.begin();

  WiFi.softAP(name.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());
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