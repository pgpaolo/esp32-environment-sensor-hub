#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "RuntimeData.h"
#include "ConfigStore.h"
#include "MqttManager.h"
#include "SensorHub.h"

class WebUi {
 public:
  WebUi(AppConfig &cfg, RuntimeData &data, ConfigStore &store, MqttManager &mqtt, SensorHub &sensors);
  void begin();
  void loop();

 private:
  AppConfig &_cfg;
  RuntimeData &_data;
  ConfigStore &_store;
  MqttManager &_mqtt;
  SensorHub &_sensors;
  WebServer _server;

  bool auth();
  String esc(const String &s) const;
  String chk(bool v) const;
  String sel(bool v) const;
  String pageStart(const String &title) const;
  String pageEnd() const;
  void handleRoot();
  void handleApiStatus();
  void handleConfig();
  void handleSave();
  void handleFactory();
  void setupOta();
  static uint8_t parseHexByte(const String &value, uint8_t fallback);
  static uint32_t nowEpoch();
};
