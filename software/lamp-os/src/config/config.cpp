#include "./config.hpp"

#include <ArduinoJson.h>

namespace lamp {
Config::Config(JsonDocument inJson) {};

JsonDocument Config::asJsonDocument() {
  JsonDocument doc;

  doc["lamp"] = JsonObject();
  doc["lamp"]["name"] = lamp.name;
  doc["lamp"]["brightness"] = lamp.brightness;
  doc["lamp"]["homeMode"] = lamp.homeMode;

  doc["base"] = JsonObject();
  doc["base"]["px"] = base.px;
  doc["base"]["ac"] = base.ac;
  doc["base"]["colors"] = JsonArray();
  for (int i = 0; i < base.colors.size(); i++) {
    doc["base"]["colors"][i] = colorToHexString(base.colors[i]);
  }
  doc["base"]["knockout"] = JsonArray();
  for (int i = 0; i < base.knockoutPixels.size(); i++) {
    doc["base"]["knockout"][i]["p"] = base.knockoutPixels[i].p;
    doc["base"]["knockout"][i]["b"] = base.knockoutPixels[i].b;
  }

  doc["shade"] = JsonObject();
  doc["shade"]["px"] = shade.px;
  doc["shade"]["colors"] = JsonArray();
  for (int i = 0; i < shade.colors.size(); i++) {
    doc["shade"]["colors"][i] = colorToHexString(shade.colors[i]);
  }

  return doc;
};
}  // namespace lamp