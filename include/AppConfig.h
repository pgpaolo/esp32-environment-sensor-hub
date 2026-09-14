#pragma once

#include <Arduino.h>
#if __has_include("DefaultSecrets.h")
  #include "DefaultSecrets.h"
#else
  #define DEFAULT_WIFI_SSID ""
  #define DEFAULT_WIFI_PASSWORD ""
#endif

enum class UvCalibrationMode : uint8_t {
  CustomLinear = 0,
  Guva100mVPerUvi = 1
};

enum class Ina219Calibration : uint8_t {
  Range32V2A = 0,
  Range32V1A = 1,
  Range16V400mA = 2
};

struct AppConfig {
  String deviceName = "esp32-sensor";
  String wifiSsid = DEFAULT_WIFI_SSID;
  String wifiPassword = DEFAULT_WIFI_PASSWORD;
  bool wifiStaticIp = false;
  String wifiIp = "192.168.1.221";
  String wifiGateway = "192.168.1.1";
  String wifiSubnet = "255.255.255.0";
  String wifiDns1 = "192.168.1.1";
  String wifiDns2 = "1.1.1.1";
  String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

  String webUser = "admin";
  String webPassword;

  String mqttHost;
  uint16_t mqttPort = 1883;
  String mqttUser;
  String mqttPassword;
  String mqttBaseTopic = "sensors/esp32-sensor";
  bool mqttRetain = false;
  bool mqttTls = false;
  bool mqttTlsInsecure = true;
  String mqttCaCert;
  uint16_t mqttReconnectSec = 5;

  uint32_t sensorIntervalSec = 30;
  uint32_t telemetryIntervalSec = 60;

  uint8_t i2cSda = 21;
  uint8_t i2cScl = 22;

  bool statusLedEnabled = true;
  uint8_t statusLedPin = 12;
  bool statusLedInverted = false;
  bool relayEnabled = true;
  uint8_t relayPin = 15;
  bool relayInverted = false;

  bool bh1750Enabled = true;
  uint8_t bh1750Address = 0x23;
  float bh1750OffsetLux = 0.0f;

  bool bmeEnabled = true;
  uint8_t bmeAddress = 0x76;
  float bmePressureOffsetHpa = 0.0f;
  float bmeTemperatureOffsetC = 0.0f;
  float bmeHumidityOffsetPct = 0.0f;

  bool dhtEnabled = true;
  uint8_t dhtPin = 14;
  float dhtTemperatureOffsetC = 0.0f;
  float dhtHumidityOffsetPct = 0.0f;

  bool inaEnabled = true;
  uint8_t inaAddress = 0x40;
  Ina219Calibration inaCalibration = Ina219Calibration::Range32V2A;
  float inaBusVoltageOffsetV = 0.0f;
  float inaCurrentOffsetMa = 0.0f;

  bool sdsEnabled = true;
  uint8_t sdsRxPin = 16;
  uint8_t sdsTxPin = 17;
  uint16_t sdsCycleMinutes = 60;
  uint16_t sdsWarmupSec = 30;
  uint8_t sdsSamples = 5;
  uint16_t sdsSampleGapMs = 1000;
  uint16_t sdsMaxAwakeSec = 120;
  uint16_t sdsFirstCycleDelaySec = 10;

  bool uvEnabled = true;
  uint8_t uvPin = 34;
  UvCalibrationMode uvCalibrationMode = UvCalibrationMode::Guva100mVPerUvi;
  float uvZeroMv = 0.0f;
  float uvMvPerIndex = 100.0f;
  uint8_t uvSamples = 32;
  uint16_t uvSampleGapUs = 1500;
  float uvMaxIndex = 20.0f;

  bool as3935Enabled = true;
  uint8_t as3935Address = 0x03;
  uint8_t as3935IrqPin = 27;
  bool as3935Outdoor = true;
  uint8_t as3935NoiseFloor = 2;
  uint8_t as3935Watchdog = 2;
  uint8_t as3935SpikeRejection = 2;
  uint8_t as3935LightningThreshold = 1;
  bool as3935MaskDisturber = false;

  uint8_t configButtonPin = 0;
};
