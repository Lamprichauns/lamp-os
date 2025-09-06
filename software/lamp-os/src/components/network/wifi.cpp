#include "./wifi.hpp"

#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "../../secrets.hpp"
#include "../../util/color.hpp"
#include "./artnet.hpp"
#include "./wifi.hpp"
#include "SPIFFS.h"

namespace lamp {
ArtnetWifi artnet;
static DNSServer dnsServer;
static AsyncWebServer server(80);
static AsyncWebSocketMessageHandler wsHandler;
static AsyncWebSocket ws("/ws", wsHandler.eventHandler());

#ifdef LAMP_DEBUG
void wsMonitor() {
  wsHandler.onConnect([](AsyncWebSocket *server, AsyncWebSocketClient *client) {
    Serial.printf("Client %" PRIu32 " connected\n", client->id());
  });

  wsHandler.onDisconnect([](AsyncWebSocket *server, uint32_t clientId) {
    Serial.printf("Client %" PRIu32 " disconnected\n", clientId);
  });

  wsHandler.onError([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                       uint16_t errorCode, const char *reason, size_t len) {
    Serial.printf("Client %" PRIu32 " error: %" PRIu16 ": %s\n", client->id(),
                  errorCode, reason);
  });

  wsHandler.onFragment([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                          const AwsFrameInfo *frameInfo, const uint8_t *data,
                          size_t len) {
    Serial.printf("Client %" PRIu32 " fragment %" PRIu32 ": %s\n", client->id(),
                  frameInfo->num, (const char *)data);
  });
}
#endif

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
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.onEvent(onWiFiEvent);
  WiFi.softAP(name.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());

#ifdef LAMP_DEBUG
  wsMonitor();
#endif
  wsHandler.onMessage([&](AsyncWebSocket *server, AsyncWebSocketClient *client,
                          const uint8_t *data, size_t len) {
    Serial.printf("Client %" PRIu32 " data: %s\n", client->id(),
                  (const char *)data);
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/configurator.html", String(), false);
  });
  server.addHandler(&ws);
  server.begin();
  artnet.begin();
};

void WifiComponent::tick() {
  dnsServer.processNextRequest();
  uint32_t now = millis();

  if (now > lastWebSocketCleanTimeMs + WEBSOCKET_CLEAN_TIME_MS) {
    ws.cleanupClients(1);
    lastWebSocketCleanTimeMs = now;
#ifdef LAMP_DEBUG
    Serial.printf("WS free heap: %" PRIu32 "\n", ESP.getFreeHeap());
#endif
  }
};

bool WifiComponent::hasArtnetData() { return artnet.newDmxData; }

std::vector<Color> WifiComponent::getArtnetData() {
  artnet.newDmxData = false;
  return artnet.artnetData;
};

unsigned long WifiComponent::getLastArtnetFrameTimeMs() {
  return artnet.lastDmxFrameMs;
};
}  // namespace lamp