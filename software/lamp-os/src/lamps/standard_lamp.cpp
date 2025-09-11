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
#include "./util/levels.hpp"
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

/**
 * - Initialize all of the lamps behaviors
 * - Initialize the animation compositor
 */
void initBehaviors() {
  shadeDmxBehavior = lamp::DmxBehavior(&shade, 0);
  baseDmxBehavior = lamp::DmxBehavior(&base, 0);
  shadeSocialBehavior = lamp::SocialBehavior(&shade, 1200);
  shadeSocialBehavior.setBluetoothComponent(&bt);
  shadeConfiguratorBehavior = lamp::ConfiguratorBehavior(&shade, 120);
  shadeConfiguratorBehavior.colors = std::vector<lamp::Color>{shade.defaultColor};
  baseConfiguratorBehavior = lamp::ConfiguratorBehavior(&base, 120);
  baseConfiguratorBehavior.colors = std::vector<lamp::Color>{base.defaultColor};
  // baseConfiguratorBehavior.knockoutPixels = config.base.knockoutPixels;
  shadeFadeOutBehavior = lamp::FadeOutBehavior(&shade, REBOOT_ANIMATION_FRAMES);
  shadeFadeOutBehavior.setWifiComponent(&wifi);
  baseFadeOutBehavior = lamp::FadeOutBehavior(&base, REBOOT_ANIMATION_FRAMES);
  baseFadeOutBehavior.setWifiComponent(&wifi);

  // layers load in priority sequence {lowest, ..., highest}
  compositor.begin({&shadeSocialBehavior,
                    &shadeConfiguratorBehavior,
                    &baseConfiguratorBehavior,
                    &baseFadeOutBehavior,
                    &shadeFadeOutBehavior},
                   {&shade, &base});
}

/**
 * ArtNet DMX actions shared between the base and shade
 */
void handleArtnet() {
  if (wifi.hasArtnetData()) {
    std::vector<lamp::Color> artnetData = wifi.getArtnetData();
    shadeDmxBehavior.setColor(artnetData[0]);
    shadeDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
    baseDmxBehavior.setColor(artnetData[1]);
    baseDmxBehavior.setLastArtnetFrameTimeMs(wifi.getLastArtnetFrameTimeMs());
  }
};

/**
 * Whole lamp changes from the configuration tool
 */
void handleWebSocket() {
  if (wifi.hasWebSocketData()) {
    JsonDocument doc = wifi.getWebSocketData();
    shadeConfiguratorBehavior.lastWebSocketUpdateTimeMs = wifi.getLastWebSocketUpdateTimeMs();
    baseConfiguratorBehavior.lastWebSocketUpdateTimeMs = wifi.getLastWebSocketUpdateTimeMs();

    // parse the ws action id (a) into a String
    String action = String(doc["a"]);
    if (action == "bright") {
      int level = doc["v"] | 100;
      shadeStrip.setBrightness(lamp::calculateBrightnessLevel(LAMP_MAX_BRIGHTNESS, level));
      baseStrip.setBrightness(lamp::calculateBrightnessLevel(LAMP_MAX_BRIGHTNESS, level));
    } else if (action == "knockout") {
      baseConfiguratorBehavior.knockoutPixels[uint8_t(doc["p"] | 0)] = uint8_t(doc["b"] | 100);
    } else if (action == "base") {
      JsonArray baseColors = doc["c"];
      if (baseColors.size()) {
        baseConfiguratorBehavior.colors.clear();
        for (JsonVariant baseColor : baseColors) {
          baseConfiguratorBehavior.colors.push_back(lamp::hexStringToColor(baseColor));
        }
      }
    } else if (action == "shade") {
      JsonArray shadeColors = doc["c"];
      if (shadeColors.size()) {
        shadeConfiguratorBehavior.colors.clear();
        for (JsonVariant shadeColor : shadeColors) {
          shadeConfiguratorBehavior.colors.push_back(lamp::hexStringToColor(shadeColor));
        }
      }
    }
  }
}

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  SPIFFS.begin(true);
  lamp::Config config = lamp::Config(&prefs);
  bt.begin(config.lamp.name, config.base.colors[0], config.shade.colors[0]);
  wifi.begin(&config);
  shadeStrip.setBrightness(lamp::calculateBrightnessLevel(LAMP_MAX_BRIGHTNESS, config.lamp.brightness));
  baseStrip.setBrightness(lamp::calculateBrightnessLevel(LAMP_MAX_BRIGHTNESS, config.lamp.brightness));
  shade.begin(config.shade.colors[0], config.shade.px, &shadeStrip);
  base.begin(config.base.colors.at(config.base.ac), config.base.px, &baseStrip);
  initBehaviors();
};

void loop() {
  handleArtnet();
  handleWebSocket();
  wifi.tick();
  compositor.tick();
};