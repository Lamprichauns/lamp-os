#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "wifi.hpp"

AsyncWebServer server(80);
DNSServer dnsServer;
bool apActive = false;

void init(const String ssid, const String password)
{
  WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  WiFi.setTxPower(wifi_power_t(WIFI_POWER_19_5dBm));

  if (!apActive) // start captive portal if AP active
  {
    server.begin();

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
  }

  apActive = true;
}