#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "AppConfig.h"

class ConfigStore {
 public:
  bool load(AppConfig &cfg);
  bool save(const AppConfig &cfg);
  void clear();
};
