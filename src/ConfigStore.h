#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "AppConfig.h"

class ConfigStore {
 public:
  static constexpr uint16_t CURRENT_SCHEMA_VERSION = 1;

  bool load(AppConfig &cfg);
  bool save(const AppConfig &cfg);
  void clear();

  static bool validate(AppConfig &cfg);
  static uint16_t schemaVersion() { return CURRENT_SCHEMA_VERSION; }

 private:
  static bool validGpio(uint8_t pin);
  static bool validOutputGpio(uint8_t pin);
  static bool validAdc1Gpio(uint8_t pin);
};
