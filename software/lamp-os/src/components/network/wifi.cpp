#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "./wifi.hpp"
#include "SPIFFS.h"

namespace lamp {
  DNSServer dnsServer;
  AsyncWebServer server(80);

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
    WiFi.softAP(name.c_str());
    dnsServer.start(53, "*", WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server.begin();
  };

  void WifiComponent::tick(){
    dnsServer.processNextRequest();
  }
}