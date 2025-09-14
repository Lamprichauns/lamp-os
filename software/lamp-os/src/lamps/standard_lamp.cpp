#include "./standard_lamp.hpp"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Preferences.h>

#include <cstdint>
#include <string>

#include "../components/network/bluetooth.hpp"
#include "../components/network/wifi.hpp"
#include "../util/color.hpp"
#include "./behaviors/configurator.hpp"
#include "./behaviors/dmx.hpp"
#include "./behaviors/fade_out.hpp"
#include "./behaviors/knockout.hpp"
#include "./behaviors/social.hpp"
#include "./config/config.hpp"
#include "./core/animated_behavior.hpp"
#include "./core/compositor.hpp"
#include "./core/frame_buffer.hpp"
#include "./globals.hpp"
#include "./util/color.hpp"
#include "./util/gradient.hpp"
#include "./util/levels.hpp"
#include "SPIFFS.h"

Adafruit_NeoPixel shadeStrip(LAMP_MAX_STRIP_PIXELS_SHADE, LAMP_SHADE_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel baseStrip(LAMP_MAX_STRIP_PIXELS_BASE, LAMP_BASE_PIN, NEO_GRBW + NEO_KHZ800);
Preferences prefs;
uint32_t lastStageModeCheckTimeMs = 0;
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
lamp::KnockoutBehavior baseKnockoutBehavior;
lamp::Config config;

/**
 * - Initialize all of the lamps behaviors
 * - Initialize the animation compositor
 */
void initBehaviors(lamp::Config* config) {
  shadeDmxBehavior = lamp::DmxBehavior(&shade, 0, true);
  baseDmxBehavior = lamp::DmxBehavior(&base, 0, true);
  shadeSocialBehavior = lamp::SocialBehavior(&shade, 1200);
  shadeSocialBehavior.setBluetoothComponent(&bt);
  shadeConfiguratorBehavior = lamp::ConfiguratorBehavior(&shade, 120);
  shadeConfiguratorBehavior.colors = shade.defaultColors;
  baseConfiguratorBehavior = lamp::ConfiguratorBehavior(&base, 120);
  baseConfiguratorBehavior.colors = base.defaultColors;
  shadeFadeOutBehavior = lamp::FadeOutBehavior(&shade, REBOOT_ANIMATION_FRAMES);
  shadeFadeOutBehavior.setWifiComponent(&wifi);
  baseFadeOutBehavior = lamp::FadeOutBehavior(&base, REBOOT_ANIMATION_FRAMES);
  baseFadeOutBehavior.setWifiComponent(&wifi);
  baseKnockoutBehavior = lamp::KnockoutBehavior(&base, 0, true);
  baseKnockoutBehavior.knockoutPixels = config->base.knockoutPixels;

  // layers load in priority sequence {lowest, ..., highest}
  compositor.begin({&baseDmxBehavior,
                    &shadeDmxBehavior,
                    &shadeSocialBehavior,
                    &shadeConfiguratorBehavior,
                    &baseConfiguratorBehavior,
                    &baseFadeOutBehavior,
                    &shadeFadeOutBehavior},
                   {&shade, &base},
                   config->lamp.homeMode);

  compositor.overlayBehaviors.push_back(&baseKnockoutBehavior);
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
 * Handle wifi mode swaps when a stage router is present
 */
void handleStageMode() {
  uint32_t now = millis();

  if (config.stage.enabled && now > lastStageModeCheckTimeMs + 2000) {
    lastStageModeCheckTimeMs = now;

    if (!wifi.stageMode) {
      auto foundStages = bt.getStages();

      if (foundStages->size() > 0) {
        wifi.toStageMode(foundStages->at(0).ssid, foundStages->at(0).password);
      }
    }
  }
}

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
      int pixelIndex = doc["p"];
      int percentage = doc["b"];
      if (pixelIndex >= 0 && pixelIndex < 50 && percentage >= 0 && percentage <= 100) {
        baseKnockoutBehavior.knockoutPixels[pixelIndex] = percentage;
      }
    } else if (action == "base") {
      JsonArray baseColors = doc["c"];
      if (baseColors.size()) {
        std::vector<lamp::Color> updatedColors;
        for (JsonVariant baseColor : baseColors) {
          updatedColors.push_back(lamp::hexStringToColor(baseColor));
        }
        baseConfiguratorBehavior.colors = lamp::buildGradientWithStops(base.pixelCount, updatedColors);
      }
    } else if (action == "shade") {
      JsonArray shadeColors = doc["c"];
      if (shadeColors.size()) {
        std::vector<lamp::Color> updatedColors;
        for (JsonVariant shadeColor : shadeColors) {
          updatedColors.push_back(lamp::hexStringToColor(shadeColor));
        }
        shadeConfiguratorBehavior.colors = lamp::buildGradientWithStops(shade.pixelCount, updatedColors);
      }
    }
  }
}

void setup() {
#ifdef LAMP_DEBUG
  Serial.begin(115200);
#endif
  config = lamp::Config(&prefs);
  SPIFFS.begin(true);
  bt.begin(config.lamp.name, config.base.colors[config.base.ac], config.shade.colors[0]);
  wifi.begin(&config);
  shadeStrip.setBrightness(lamp::calculateBrightnessLevel(LAMP_MAX_BRIGHTNESS, config.lamp.brightness));
  baseStrip.setBrightness(lamp::calculateBrightnessLevel(LAMP_MAX_BRIGHTNESS, config.lamp.brightness));
  shade.begin(lamp::buildGradientWithStops(config.shade.px, config.shade.colors), config.shade.px, &shadeStrip);
  base.begin(lamp::buildGradientWithStops(config.base.px, config.base.colors), config.base.px, &baseStrip);
  initBehaviors(&config);
};

void loop() {
  handleStageMode();
  handleArtnet();
  handleWebSocket();
  wifi.tick();
  compositor.tick();
};