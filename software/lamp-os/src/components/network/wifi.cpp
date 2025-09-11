#include "./wifi.hpp"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>

#include "../../config/config.hpp"
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
Preferences prefs;

#ifdef LAMP_DEBUG
void wsMonitor() {
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
    request->send(SPIFFS, "/index.html.gz", String(), false);
  };
};

WifiComponent::WifiComponent() {};

void WifiComponent::begin(Config *inConfig) {
#ifdef LAMP_DEBUG
  Serial.printf("Starting Wifi Async Client\n");
#endif
  Serial.begin(115200);
  config = inConfig;
  serializeJson(config->asJsonDocument(), doc);
  WiFi.mode(WIFI_AP);
  WiFi.onEvent(onWiFiEvent);
  WiFi.softAP(inConfig->lamp.name.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());

#ifdef LAMP_DEBUG
  wsMonitor();
#endif
  wsHandler.onMessage([&](AsyncWebSocket *server, AsyncWebSocketClient *client, const uint8_t *data, size_t len) {
#ifdef LAMP_DEBUG
    Serial.printf("Client %" PRIu32 " data: %s\n", client->id(),
                  (const char *)data);
#endif
    lastWebSocketUpdateTimeMs = millis();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
#ifdef LAMP_DEBUG
      Serial.print("ws deserializeJson() failed: ");
      Serial.println(error.c_str());
#endif
      return;  // use class defaults
    }

    newWebSocketData = true;
    lastWebSocketData = doc;
  });
  wsHandler.onConnect([&](AsyncWebSocket *server, AsyncWebSocketClient *client) {
#ifdef LAMP_DEBUG
    Serial.printf("Client %" PRIu32 " connected\n", client->id());
#endif
    lastWebSocketUpdateTimeMs = millis();
  });
  wsHandler.onDisconnect([&](AsyncWebSocket *server, uint32_t clientId) {
#ifdef LAMP_DEBUG
    Serial.printf("Client %" PRIu32 " disconnected\n", clientId);
#endif
    lastWebSocketUpdateTimeMs = millis();
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(doc.c_str());
    request->send(response);
  });
  server.on(
      "/settings",
      HTTP_PUT,
      nullptr,
      nullptr,
      [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        try {
          String buf;
          for (size_t i = 0; i < len; i++) {
            buf.concat((char)data[i]);
          }
          prefs.begin("lamp", false);
          size_t status = prefs.putString("cfg", buf);
          prefs.end();

          if (status) {
            requiresReboot = true;
            request->send(200);
            return;
          }
        } catch (int e) {
          request->send(500);
          return;
        }

        request->send(500);
        return;
      });
  server.addHandler(&ws);
  server.begin();
  artnet.begin();
};

void WifiComponent::tick() {
  dnsServer.processNextRequest();
  uint32_t now = millis();

  if (now > lastWebSocketCleanTimeMs + WEBSOCKET_CLEAN_TIME_MS &&
      now > lastWebSocketUpdateTimeMs + WEBSOCKET_CLEAN_TIME_MS) {
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

bool WifiComponent::hasWebSocketData() { return newWebSocketData; };

unsigned long WifiComponent::getLastWebSocketUpdateTimeMs() {
  return lastWebSocketUpdateTimeMs;
};

JsonDocument WifiComponent::getWebSocketData() {
  newWebSocketData = false;
  return lastWebSocketData;
};
}  // namespace lamp