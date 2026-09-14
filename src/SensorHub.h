#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MAX31865.h>
#include <Adafruit_ADS1X15.h>
#include <DHT.h>
#include <SdsDustSensor.h>
#include <SparkFun_AS3935.h>

#include "AppConfig.h"
#include "RuntimeData.h"

class SensorHub {
 public:
  SensorHub();
  ~SensorHub();

  void begin(AppConfig &cfg, RuntimeData &data);
  void beginNesaSensors();
  void tick();
  void sampleSlowSensors();
  void sampleNesaSensors();
  bool takeImmediatePublishFlag();
  bool requestSdsMeasurement();
  bool forceSdsSleep(const char *reason = "manual_sleep");
  bool setRelay(bool on);
  bool toggleRelay();
  String scanI2c();

 private:
  AppConfig *_cfg = nullptr;
  RuntimeData *_data = nullptr;

  BH1750 _bh1750;
  Adafruit_BME280 _bme;
  Adafruit_INA219 *_ina = nullptr;
  Adafruit_MAX31865 *_nesaTa = nullptr;
  Adafruit_ADS1115 *_nesaRsg1 = nullptr;
  bool _nesaTaInitialized = false;
  bool _nesaRsg1Initialized = false;
  DHT *_dht = nullptr;
  uint32_t _dhtReadyAtMs = 0;

  HardwareSerial &_sdsSerial;
  SdsDustSensor _sds;
  enum class SdsState { Disabled, Sleeping, Warming, Sampling };
  SdsState _sdsState = SdsState::Disabled;
  uint32_t _sdsNextCycleMs = 0;
  uint32_t _sdsDeadlineMs = 0;
  uint32_t _sdsNextSampleMs = 0;
  uint32_t _sdsAwakeStartMs = 0;
  uint8_t _sdsCollected = 0;
  uint8_t _sdsAttempts = 0;
  float _sdsPm25Sum = 0;
  float _sdsPm10Sum = 0;

  SparkFun_AS3935 *_lightning = nullptr;
  volatile bool _asIrq = false;
  bool _immediatePublish = false;
  uint32_t _asLastInitAttemptMs = 0;

  static SensorHub *_instance;
  static void IRAM_ATTR as3935Isr();

  void beginGpio();
  void beginBh1750();
  void beginBme();
  void beginDht();
  void beginIna();
  void beginNesaTa();
  void beginNesaRsg1();
  void beginSds();
  void beginAs3935();
  void tickSds();
  void tickAs3935();
  void sampleDht();
  void sampleUv();
  void sampleIna();
  void sampleNesaTa();
  void sampleNesaRsg1();
  void startSdsCycle();
  void finishSdsCycle(bool success, const String &error = "");
  void scheduleNextSds(uint32_t fromMs);
  bool i2cPresent(uint8_t address);
  static float dewPointC(float tempC, float humidityPct);
  static uint32_t nowEpoch();
};
