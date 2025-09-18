#include "./config.hpp"

#include <ArduinoJson.h>
#include <Preferences.h>

#include "../util/color.hpp"

namespace lamp {
Config::Config(Preferences* inPrefs) {
  JsonDocument doc;
  prefs = inPrefs;
  prefs->begin("lamp", true);
  String json = prefs->getString("cfg", "{}");
  DeserializationError error = deserializeJson(doc, json);
  prefs->end();

#ifdef LAMP_DEBUG
  Serial.println(json);
#endif

  if (error) {
#ifdef LAMP_DEBUG
    Serial.printf("ws deserializeJson() failed: %s\n", error.c_str());
#endif
    return;  // use class defaults
  }

  JsonObject lampNode = doc["lamp"];
  lamp.name = std::string(lampNode["name"] | "standard");
  lamp.brightness = lampNode["brightness"] | 100;
  lamp.homeMode = lampNode["homeMode"] | false;
  lamp.homeModeSSID = std::string(lampNode["homeModeSSID"] | "");
  lamp.homeModeBrightness = lampNode["homeModeBrightness"] | 80;
  lamp.webPassword = std::string(lampNode["webPassword"] | "");

  JsonObject baseNode = doc["base"];
  base.px = baseNode["px"] | 36;
  if (base.px > 50) {
    base.px = 50;
  }
  base.ac = baseNode["ac"] | 0;

  JsonArray baseColors = baseNode["colors"];
  int colorCollectionSize = baseColors.size();
  if (base.ac > colorCollectionSize - 1) {
    base.ac = 0;
  }

  if (colorCollectionSize > 0) {
    base.colors.clear();
    for (JsonVariant baseColor : baseColors) {
      base.colors.push_back(hexStringToColor(baseColor));
    }
  }
  JsonArray baseKnockoutPixels = baseNode["knockout"];
  if (baseKnockoutPixels.size()) {
    for (JsonObject baseKnockoutPixel : baseKnockoutPixels) {
      int pixelIndex = baseKnockoutPixel["p"] | 0;
      if (pixelIndex > 49) {
        continue;
      }

      base.knockoutPixels[pixelIndex] = baseKnockoutPixel["b"] | 100;
    }
  }

  JsonObject shadeNode = doc["shade"];
  JsonArray shadeColors = shadeNode["colors"];
  if (shadeColors.size()) {
    shade.colors.clear();
    for (JsonVariant shadeColor : shadeColors) {
      shade.colors.push_back(hexStringToColor(shadeColor));
    }
  }
};

JsonDocument Config::asJsonDocument() {
  JsonDocument doc;

  JsonObject lampNode = doc["lamp"].to<JsonObject>();
  lampNode["name"] = lamp.name;
  lampNode["brightness"] = lamp.brightness;
  lampNode["homeMode"] = lamp.homeMode;
  lampNode["homeModeSSID"] = lamp.homeModeSSID;
  lampNode["homeModeBrightness"] = lamp.homeModeBrightness;
  lampNode["webPassword"] = lamp.webPassword;

  JsonObject baseNode = doc["base"].to<JsonObject>();
  baseNode["px"] = base.px;
  baseNode["ac"] = base.ac;
  JsonArray baseColorsNode = baseNode["colors"].to<JsonArray>();
  for (int i = 0; i < base.colors.size(); i++) {
    baseColorsNode[i] = colorToHexString(base.colors[i]);
  }
  JsonArray baseKnockoutNode = baseNode["knockout"].to<JsonArray>();
  int currentIdx = 0;
  for (int i = 0; i < base.knockoutPixels.size(); i++) {
    int value = base.knockoutPixels[i];

    // only send values that aren't 100% brightness as an optimization
    if (value == 100) {
      continue;
    }

    JsonObject baseKnockoutObjectNode = baseKnockoutNode[currentIdx].to<JsonObject>();
    baseKnockoutObjectNode["p"] = i;
    baseKnockoutObjectNode["b"] = value;

    currentIdx++;
  }

  JsonObject shadeNode = doc["shade"].to<JsonObject>();
  shadeNode["px"] = shade.px;
  JsonArray shadeColorsNode = shadeNode["colors"].to<JsonArray>();
  for (int i = 0; i < shade.colors.size(); i++) {
    shadeColorsNode[i] = colorToHexString(shade.colors[i]);
  }

  return doc;
};

}  // namespace lamp