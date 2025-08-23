#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "./wifi.hpp"
#include "SPIFFS.h"
#include "ArtnetWifi.h"

namespace lamp {
  DNSServer dnsServer;
  AsyncWebServer server(80);
  WiFiUDP UdpSend;
  ArtnetWifi artnet;

  void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
  {
    bool tail = false;
    Serial.print("DMX: Univ: ");
    Serial.print(universe, DEC);
    Serial.print(", Seq: ");
    Serial.print(sequence, DEC);
    Serial.print(", Data (");
    Serial.print(length, DEC);
    Serial.print("): ");

    if (length > 16) {
      length = 16;
      tail = true;
    }
    // send out the buffer
    for (uint16_t i = 0; i < length; i++)
    {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    if (tail) {
      Serial.print("...");
    }
    Serial.println();
  }

  class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {};
    virtual ~CaptiveRequestHandler() {};

    bool canHandle(AsyncWebServerRequest *request){
      return true;
    };

    void handleRequest(AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/configurator.html", String(), false);
    };
  };

  void WifiComponent::begin(std::__cxx11::string name) {
    WiFi.begin(COORDINATOR_SSID, COORDINATOR_SHARED_PASS);
    artnet.setArtDmxCallback(onDmxFrame);
    artnet.begin();

    WiFi.softAP(name.c_str());
    dnsServer.start(53, "*", WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server.begin();
  };

  void WifiComponent::tick(){
    dnsServer.processNextRequest();
    artnet.read();
  }
}