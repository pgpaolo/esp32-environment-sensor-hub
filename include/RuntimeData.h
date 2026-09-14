#pragma once

#include <Arduino.h>

struct RuntimeData {
  bool relayState = false;

  bool bh1750Ok = false;
  float bh1750Lux = NAN;
  uint32_t bh1750Failures = 0;
  uint8_t bh1750DetectedAddress = 0xFF;
  String bh1750LastError;

  bool bmeOk = false;
  float bmeTempC = NAN;
  float bmeHumidity = NAN;
  float bmePressureHpa = NAN;
  float bmeDewPointC = NAN;
  uint32_t bmeFailures = 0;
  uint8_t bmeDetectedAddress = 0xFF;
  String bmeLastError;

  bool dhtOk = false;
  float dhtTempC = NAN;
  float dhtHumidity = NAN;
  float dhtDewPointC = NAN;
  uint32_t dhtFailures = 0;
  uint32_t dhtLastSuccessMs = 0;
  String dhtLastError;

  bool inaOk = false;
  float inaBusVoltageV = NAN;
  float inaShuntVoltageMv = NAN;
  float inaLoadVoltageV = NAN;
  float inaCurrentMa = NAN;
  float inaPowerMw = NAN;
  uint32_t inaFailures = 0;
  uint8_t inaDetectedAddress = 0xFF;
  String inaLastError;

  bool nesaTaOk = false;
  float nesaTaTemperatureC = NAN;
  float nesaTaResistanceOhm = NAN;
  uint8_t nesaTaFault = 0;
  uint32_t nesaTaFailures = 0;
  String nesaTaLastError;

  bool nesaRsg1Ok = false;
  int16_t nesaRsg1Raw = 0;
  float nesaRsg1MilliVolts = NAN;
  float nesaRsg1RadiationWm2 = NAN;
  uint8_t nesaRsg1DetectedAddress = 0xFF;
  uint32_t nesaRsg1Failures = 0;
  String nesaRsg1LastError;

  bool uvOk = false;
  bool uvConsistent = true;
  uint16_t uvRawAdc = 0;
  uint32_t uvMilliVolts = 0;
  float uvIndex = NAN;
  uint32_t uvFailures = 0;
  String uvLastError;

  bool sdsOk = false;
  float pm25 = NAN;
  float pm10 = NAN;
  uint32_t sdsLastSampleMs = 0;
  uint32_t sdsLastSampleEpoch = 0;
  uint32_t sdsNextInSec = 0;
  uint32_t sdsStageRemainingSec = 0;
  String sdsState = "disabled";
  String sdsLastError;
  uint32_t sdsSuccessfulCycles = 0;
  uint32_t sdsFailedCycles = 0;
  uint8_t sdsSamplesCollected = 0;
  uint8_t sdsAttempts = 0;

  bool as3935Ok = false;
  String as3935LastEvent = "disabled";
  String as3935LastError;
  uint8_t as3935DetectedAddress = 0xFF;
  int as3935DistanceKm = -1;
  long as3935Energy = 0;
  uint32_t as3935EventCount = 0;
  uint32_t as3935NoiseCount = 0;
  uint32_t as3935DisturberCount = 0;
  uint32_t as3935LastEventEpoch = 0;

  int wifiRssi = 0;
  String wifiIp;
  bool mqttConnected = false;
  int mqttState = 0;
  uint32_t mqttReconnects = 0;
  uint32_t lastTelemetryEpoch = 0;
  uint32_t bootCount = 0;
};
