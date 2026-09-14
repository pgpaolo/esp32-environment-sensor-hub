#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "RuntimeData.h"

class MqttManager {
 public:
  ~MqttManager();
  void begin(AppConfig &cfg, RuntimeData &data);
  void loop();
  bool connected() const;
  int state() const;
  bool publishTelemetry(const char *reason = "periodic");
  bool publishAvailability(bool online);

 private:
  AppConfig *_cfg = nullptr;
  RuntimeData *_data = nullptr;
  WiFiClient _plain;
  WiFiClientSecure _secure;
  PubSubClient *_mqtt = nullptr;
  uint32_t _lastConnectAttemptMs = 0;
  uint16_t _currentBackoffSec = 0;
  bool _lastObservedConnected = false;

  bool ensureConnected();
  void observeConnectionState();
  void registerPublishResult(bool ok);
  String topic(const char *suffix) const;
  static uint32_t nowEpoch();
};
