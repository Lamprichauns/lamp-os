#include <Arduino.h>
#include <AsyncUDP.h>
#include <NimBLEDevice.h>
#include <WiFi.h>

#include <string>
#include <vector>

#include "./secrets.hpp"
#define ART_NET_PORT 6454
#define MAX_BUFFER_ARTNET 530
#define BLE_MAGIC_NUMBER 42007

#ifdef CONFIG_ARDUINO_UDP_TASK_PRIORITY
#undef CONFIG_ARDUINO_UDP_TASK_PRIORITY
#define CONFIG_ARDUINO_UDP_TASK_PRIORITY 10
#endif

#ifdef CONFIG_ARDUINO_UDP_RUNNING_CORE
#undef CONFIG_ARDUINO_UDP_RUNNING_CORE
#define CONFIG_ARDUINO_UDP_RUNNING_CORE 1
#endif

// Tx power level in DB
// @see platformio build flag MYNEWT_VAL_BLE_LL_TX_PWR_DBM as they must match
#define BLE_POWER_LEVEL 4

AsyncUDP udp;

void onWiFiEvent(WiFiEvent_t event) {
  Serial.printf("WIFI Event %d\n", event);
};

void setup() {
  Serial.begin(115200);
  std::string coordinatorSsid = SECRET_COORDINATOR_SSID;
  std::string coordinatorPassword = SECRET_COORDINATOR_PASSWORD;

  NimBLEDevice::init(SECRET_COORDINATOR_STAGE_NAME);
  NimBLEDevice::setPower(BLE_POWER_LEVEL);

  // Stage coordinators advertise the following packet
  // 2 bytes: coordinator identifier [Manufacturer ID block]
  // 26 bytes: a null terminated ssid and a null terminated password
  // combined password and ssid should not be more than 24 chars
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(SECRET_COORDINATOR_STAGE_NAME);
  pAdvertising->enableScanResponse(true);
  std::vector<unsigned char> data;
  data.reserve(28);
  std::vector<char> magicBytes{char(BLE_MAGIC_NUMBER & 0xff), char((BLE_MAGIC_NUMBER >> 8) & 0xff)};
  data.insert(data.end(), magicBytes.begin(), magicBytes.end());
  std::vector<char> ssidBytes(coordinatorSsid.c_str(), coordinatorSsid.c_str() + coordinatorSsid.size() + 1);
  data.insert(data.end(), ssidBytes.begin(), ssidBytes.end());
  std::vector<char> passwordBytes(coordinatorPassword.c_str(), coordinatorPassword.c_str() + coordinatorPassword.size() + 1);
  data.insert(data.end(), passwordBytes.begin(), passwordBytes.end());
  pAdvertising->setMinInterval(650);
  pAdvertising->setMaxInterval(800);
  pAdvertising->setManufacturerData(data);
  pAdvertising->setConnectableMode(0);
  pAdvertising->start();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.onEvent(onWiFiEvent);
  WiFi.config(IPAddress(10, 0, 0, 2), IPAddress(10, 0, 0, 1),
              IPAddress(255, 255, 255, 0), IPAddress(10, 0, 0, 1));
  WiFi.begin(SECRET_COORDINATOR_SSID, SECRET_COORDINATOR_PASSWORD, 5);
  udp.listen(ART_NET_PORT);
  udp.onPacket([](AsyncUDPPacket packet) {
    uint32_t packetSize = packet.length();

    if (packetSize == MAX_BUFFER_ARTNET) {
      uint8_t* data = packet.data();
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 20),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 21),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 22),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 23),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
      udp.writeTo(data, MAX_BUFFER_ARTNET, IPAddress(10, 0, 0, 24),
                  ART_NET_PORT, TCPIP_ADAPTER_IF_STA);
    }
  });
}

void loop() {}