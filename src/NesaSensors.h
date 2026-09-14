#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MAX31865.h>
#include <Adafruit_ADS1X15.h>
#include "AppConfig.h"
#include "RuntimeData.h"

class NesaSensors {
 public:
  ~NesaSensors();
  void begin(AppConfig &cfg, RuntimeData &data);
  void sample();

 private:
  AppConfig *_cfg = nullptr;
  RuntimeData *_data = nullptr;
  Adafruit_MAX31865 *_ta = nullptr;
  Adafruit_ADS1115 *_rsg1 = nullptr;

  void beginTa();
  void beginRsg1();
  void sampleTa();
  void sampleRsg1();
};
