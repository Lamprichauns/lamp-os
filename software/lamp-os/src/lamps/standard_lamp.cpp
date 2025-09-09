#include "./standard_lamp.hpp"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Preferences.h>

#include <string>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "./behaviors/configurator.hpp"
#include "./behaviors/dmx.hpp"
#include "./behaviors/fade_out.hpp"
#include "./behaviors/social.hpp"
#include "./config/config.hpp"
#include "./core/animated_behavior.hpp"
#include "./core/compositor.hpp"
#include "./core/frame_buffer.hpp"
#include "./globals.hpp"
#include "./util/color.hpp"
#include "SPIFFS.h"

Adafruit_NeoPixel shadeStrip(35, LAMP_SHADE_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel baseStrip(35, LAMP_BASE_PIN, NEO_GRBW + NEO_KHZ800);
Preferences prefs;
lamp::BluetoothComponent bt;
lamp::WifiComponent wifi;
lamp::Compositor compositor;
lamp::FrameBuffer shade;
lamp::FrameBuffer base;
lamp::DmxBehavior shadeDmxBehavior;
lamp::DmxBehavior baseDmxBehavior;
lamp::SocialBehavior shadeSocialBehavior;
lamp::ConfiguratorBehavior shadeConfiguratorBehavior;
lamp::ConfiguratorBehavior baseConfiguratorBehavior;
lamp::FadeOutBehavior shadeFadeOutBehavior;
lamp::FadeOutBehavior baseFadeOutBehavior;

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  lamp::Config config = lamp::Config(&prefs);
  bt.begin(config.lamp.name, config.base.colors[0], config.shade.colors[0]);
  wifi.begin(&config);
  shade.begin(config.shade.colors[0], config.shade.px, &shadeStrip);
  base.begin(config.base.colors.at(config.base.ac), config.base.px, &baseStrip);
  shadeDmxBehavior = lamp::DmxBehavior(&shade, 0);
  baseDmxBehavior = lamp::DmxBehavior(&base, 0);
  shadeSocialBehavior = lamp::SocialBehavior(&shade, 1200);
  shadeConfiguratorBehavior = lamp::ConfiguratorBehavior(&shade, 120);
  shadeConfiguratorBehavior.colors =
      std::vector<lamp::Color>{shade.defaultColor};
  baseConfiguratorBehavior = lamp::ConfiguratorBehavior(&base, 120);
  baseConfiguratorBehavior.colors = std::vector<lamp::Color>{base.defaultColor};
  shadeFadeOutBehavior = lamp::FadeOutBehavior(&shade, REBOOT_ANIMATION_FRAMES);
  baseFadeOutBehavior = lamp::FadeOutBehavior(&base, REBOOT_ANIMATION_FRAMES);
  compositor.begin(
      {&shadeSocialBehavior, &shadeConfiguratorBehavior,
       &baseConfiguratorBehavior, &baseFadeOutBehavior, &shadeFadeOutBehavior},
      {&shade, &base});
};

void loop() {
  shadeSocialBehavior.updateFoundLamps(bt.getLamps());

  if (wifi.requiresReboot) {
    shadeFadeOutBehavior.reboot = true;
    baseFadeOutBehavior.reboot = true;
  }

  if (wifi.hasWebSocketData()) {
    JsonDocument doc = wifi.getWebSocketData();
    shadeConfiguratorBehavior.lastWebSocketUpdateTimeMs =
        wifi.getLastWebSocketUpdateTimeMs();
    baseConfiguratorBehavior.lastWebSocketUpdateTimeMs =
        wifi.getLastWebSocketUpdateTimeMs();

    // parse the ws action id into a String
    String action = String(doc["a"]);
    if (action == "bright") {
      shadeConfiguratorBehavior.brightness = doc["v"] | 100;
      baseConfiguratorBehavior.brightness = doc["v"] | 100;
    } else if (action == "knockout") {
      Serial.println("knockout");
      baseConfiguratorBehavior.knockoutPixels[uint8_t(doc["p"] | 0)] =
          uint8_t(doc["b"] | 100);
    } else if (action == "base") {
      Serial.println("base");
      JsonArray baseColors = doc["c"];
      if (baseColors.size()) {
        baseConfiguratorBehavior.colors.clear();
        for (JsonVariant baseColor : baseColors) {
          baseConfiguratorBehavior.colors.push_back(
              lamp::hexStringToColor(baseColor));
        }
      }
    } else if (action == "shade") {
      Serial.println("shade");
      JsonArray shadeColors = doc["c"];
      if (shadeColors.size()) {
        baseConfiguratorBehavior.colors.clear();
        for (JsonVariant shadeColor : shadeColors) {
          shadeConfiguratorBehavior.colors.push_back(
              lamp::hexStringToColor(shadeColor));
        }
      }
    }
  }

  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    shadeDmxBehavior.setColor(artnetData[0]);
    shadeDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
    baseDmxBehavior.setColor(artnetData[1]);
    baseDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
  }
  wifi.tick();
  compositor.tick();
};