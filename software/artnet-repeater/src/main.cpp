#include <Arduino.h>
#include <AsyncUDP.h>
#include <WiFi.h>

#include "./secrets.hpp"
#define ART_NET_PORT 6454
#define MAX_BUFFER_ARTNET 530

AsyncUDP udp;

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(10, 0, 0, 2), IPAddress(10, 0, 0, 1),
              IPAddress(255, 255, 255, 0), IPAddress(10, 0, 0, 1));
  WiFi.begin(SECRET_COORDINATOR_SSID, SECRET_COORDINATOR_SHARED_PASS);
  udp.listen(ART_NET_PORT);
  udp.onPacket([](AsyncUDPPacket packet) {
    uint32_t timeStart = micros();
    uint32_t packetSize = packet.length();

    if (packetSize == MAX_BUFFER_ARTNET) {
      uint8_t* data = packet.data();
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 20),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_MAX);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 21),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 22),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_MAX);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 23),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 24),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_MAX);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 25),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 26),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_MAX);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 27),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 28),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_MAX);
    }
  });
}

void loop() {}