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
    Serial.print("cfg deserializeJson() failed: ");
    Serial.println(error.c_str());
#endif
    return;  // use class defaults
  }

  JsonObject lampNode = doc["lamp"];
  lamp.name = std::string(lampNode["name"] | "standard");
  lamp.brightness = lampNode["brightness"] | 100;
  lamp.homeMode = lampNode["homeMode"] | false;

  JsonObject baseNode = doc["base"];
  base.px = baseNode["px"] | 35;
  base.ac = baseNode["ac"] | 0;
  JsonArray baseColors = baseNode["colors"];
  if (baseColors.size()) {
    base.colors.clear();
    for (JsonVariant baseColor : baseColors) {
      base.colors.push_back(hexStringToColor(baseColor));
    }
  }

  JsonObject shadeNode = doc["shade"];
  shade.px = shadeNode["px"] | 35;
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

  JsonObject baseNode = doc["base"].to<JsonObject>();
  baseNode["px"] = base.px;
  baseNode["ac"] = base.ac;
  JsonArray baseColorsNode = baseNode["colors"].to<JsonArray>();
  for (int i = 0; i < base.colors.size(); i++) {
    baseColorsNode[i] = colorToHexString(base.colors[i]);
  }
  JsonArray baseKnockoutNode = baseNode["knockout"].to<JsonArray>();
  for (int i = 0; i < base.knockoutPixels.size(); i++) {
    JsonObject baseKnockoutObjectNode = baseKnockoutNode[i].to<JsonObject>();
    baseKnockoutObjectNode["p"] = base.knockoutPixels[i].p;
    baseKnockoutObjectNode["b"] = base.knockoutPixels[i].b;
  }

  JsonObject shadeNode = doc["shade"].to<JsonObject>();
  shadeNode["px"] = 35;
  JsonArray shadeColorsNode = shadeNode["colors"].to<JsonArray>();
  for (int i = 0; i < shade.colors.size(); i++) {
    shadeColorsNode[i] = colorToHexString(shade.colors[i]);
  }

  return doc;
};

}  // namespace lamp