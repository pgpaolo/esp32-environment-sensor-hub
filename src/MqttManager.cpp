#include "MqttManager.h"
#include <time.h>

MqttManager::~MqttManager() {
  if (_mqtt) delete _mqtt;
}

uint32_t MqttManager::nowEpoch() {
  time_t t = time(nullptr);
  return t > 1700000000 ? (uint32_t)t : 0;
}

void MqttManager::begin(AppConfig &cfg, RuntimeData &data) {
  _cfg = &cfg;
  _data = &data;

  if (cfg.mqttTls) {
    if (cfg.mqttTlsInsecure) _secure.setInsecure();
    else if (!cfg.mqttCaCert.isEmpty()) _secure.setCACert(cfg.mqttCaCert.c_str());
    _mqtt = new PubSubClient(_secure);
  } else {
    _mqtt = new PubSubClient(_plain);
  }

  _mqtt->setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
  _mqtt->setBufferSize(4096);
  _payloadBuffer.reserve(3072);

  _currentBackoffSec = cfg.mqttReconnectSec < 1 ? 1 : cfg.mqttReconnectSec;
  _data->mqttCurrentBackoffSec = _currentBackoffSec;
  _data->mqttState = _mqtt->state();
  _data->mqttLastState = _data->mqttState;
}

String MqttManager::topic(const char *suffix) const {
  String t = _cfg->mqttBaseTopic;
  while (t.endsWith("/")) t.remove(t.length() - 1);
  t += "/";
  t += suffix;
  return t;
}

void MqttManager::observeConnectionState() {
  if (!_mqtt || !_data) return;

  const bool current = _mqtt->connected();
  if (_lastObservedConnected && !current) {
    _data->mqttDisconnects++;
    _data->mqttLastDisconnectEpoch = nowEpoch();
  }

  _lastObservedConnected = current;
  _data->mqttConnected = current;
  _data->mqttState = _mqtt->state();
  _data->mqttLastState = _data->mqttState;
  _data->mqttCurrentBackoffSec = _currentBackoffSec;
}

void MqttManager::registerPublishResult(bool ok) {
  if (!_data) return;
  if (ok) {
    _data->mqttPublishOk++;
    _data->mqttLastPublishEpoch = nowEpoch();
  } else {
    _data->mqttPublishFailed++;
  }
}

bool MqttManager::ensureConnected() {
  if (!_mqtt || !_cfg || WiFi.status() != WL_CONNECTED || _cfg->mqttHost.isEmpty()) {
    if (_data) _data->mqttConnected = false;
    return false;
  }

  if (_mqtt->connected()) {
    if (_data) {
      _data->mqttConnected = true;
      _data->mqttState = 0;
      _data->mqttLastState = 0;
    }
    return true;
  }

  const uint32_t now = millis();
  const uint32_t retryMs = (uint32_t)_currentBackoffSec * 1000UL;
  if ((uint32_t)(now - _lastConnectAttemptMs) < retryMs) return false;
  _lastConnectAttemptMs = now;

  if (_data) _data->mqttConnectAttempts++;

  String clientId = _cfg->deviceName + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  String willTopic = topic("status");

  const bool ok = _cfg->mqttUser.length()
      ? _mqtt->connect(clientId.c_str(), _cfg->mqttUser.c_str(), _cfg->mqttPassword.c_str(),
                       willTopic.c_str(), 0, true, "offline")
      : _mqtt->connect(clientId.c_str(), willTopic.c_str(), 0, true, "offline");

  if (_data) {
    _data->mqttConnected = ok;
    _data->mqttState = ok ? 0 : _mqtt->state();
    _data->mqttLastState = _data->mqttState;
  }

  const uint16_t base = _cfg->mqttReconnectSec < 1 ? 1 : _cfg->mqttReconnectSec;
  if (ok) {
    _currentBackoffSec = base;
    if (_data) {
      _data->mqttConnectSuccess++;
      _data->mqttLastConnectEpoch = nowEpoch();
      _data->mqttCurrentBackoffSec = _currentBackoffSec;
    }
    publishAvailability(true);
  } else {
    uint32_t next = (uint32_t)_currentBackoffSec * 2UL;
    if (next < base) next = base;
    if (next > 60) next = 60;
    _currentBackoffSec = (uint16_t)next;
    if (_data) _data->mqttCurrentBackoffSec = _currentBackoffSec;
  }

  return ok;
}

void MqttManager::loop() {
  observeConnectionState();
  if (ensureConnected()) _mqtt->loop();
  observeConnectionState();
}

bool MqttManager::connected() const {
  return _mqtt && _mqtt->connected();
}

int MqttManager::state() const {
  return _mqtt ? _mqtt->state() : -99;
}

bool MqttManager::publishAvailability(bool online) {
  if (!_mqtt || !_mqtt->connected()) return false;
  String t = topic("status");
  const bool ok = _mqtt->publish(t.c_str(), online ? "online" : "offline", true);
  registerPublishResult(ok);
  return ok;
}

bool MqttManager::publishTelemetry(const char *reason) {
  if (!ensureConnected()) {
    registerPublishResult(false);
    return false;
  }

  JsonDocument doc;
  doc["device"] = _cfg->deviceName;
  doc["reason"] = reason;
  doc["uptime_s"] = millis() / 1000UL;
  const uint32_t epoch = nowEpoch();
  if (epoch) doc["epoch"] = epoch;

  JsonObject sys = doc["system"].to<JsonObject>();
  sys["rssi_dbm"] = WiFi.RSSI();
  sys["ip"] = WiFi.localIP().toString();
  sys["free_heap"] = ESP.getFreeHeap();
  sys["min_free_heap"] = ESP.getMinFreeHeap();
  sys["cpu_mhz"] = ESP.getCpuFreqMHz();
  sys["boot_count"] = _data->bootCount;

  JsonObject mq = sys["mqtt"].to<JsonObject>();
  mq["connect_attempts"] = _data->mqttConnectAttempts;
  mq["connect_success"] = _data->mqttConnectSuccess;
  mq["disconnects"] = _data->mqttDisconnects;
  mq["publish_ok"] = _data->mqttPublishOk;
  mq["publish_failed"] = _data->mqttPublishFailed;
  mq["state"] = _data->mqttState;
  mq["backoff_s"] = _data->mqttCurrentBackoffSec;

  JsonObject relay = doc["relay"].to<JsonObject>();
  relay["enabled"] = _cfg->relayEnabled;
  relay["state"] = _data->relayState ? "ON" : "OFF";

  JsonObject bh = doc["bh1750"].to<JsonObject>();
  bh["enabled"] = _cfg->bh1750Enabled;
  bh["ok"] = _data->bh1750Ok;
  if (_data->bh1750Ok) bh["illuminance_lux"] = _data->bh1750Lux;

  JsonObject bme = doc["bme280"].to<JsonObject>();
  bme["enabled"] = _cfg->bmeEnabled;
  bme["ok"] = _data->bmeOk;
  if (_data->bmeOk) {
    bme["temperature_c"] = _data->bmeTempC;
    bme["humidity_pct"] = _data->bmeHumidity;
    bme["pressure_hpa"] = _data->bmePressureHpa;
    bme["dewpoint_c"] = _data->bmeDewPointC;
  }

  JsonObject dht = doc["dht11"].to<JsonObject>();
  dht["enabled"] = _cfg->dhtEnabled;
  dht["ok"] = _data->dhtOk;
  if (_data->dhtOk) {
    dht["temperature_c"] = _data->dhtTempC;
    dht["humidity_pct"] = _data->dhtHumidity;
    dht["dewpoint_c"] = _data->dhtDewPointC;
  }

  JsonObject ina = doc["ina219"].to<JsonObject>();
  ina["enabled"] = _cfg->inaEnabled;
  ina["ok"] = _data->inaOk;
  if (_data->inaOk) {
    ina["bus_voltage_v"] = _data->inaBusVoltageV;
    ina["shunt_voltage_mv"] = _data->inaShuntVoltageMv;
    ina["load_voltage_v"] = _data->inaLoadVoltageV;
    ina["current_ma"] = _data->inaCurrentMa;
    ina["power_mw"] = _data->inaPowerMw;
  }

  JsonObject ta = doc["nesa_ta_n"].to<JsonObject>();
  ta["enabled"] = _cfg->nesaTaEnabled;
  ta["ok"] = _data->nesaTaOk;
  if (_data->nesaTaOk) {
    ta["temperature_c"] = _data->nesaTaTemperatureC;
    ta["resistance_ohm"] = _data->nesaTaResistanceOhm;
    ta["fault"] = _data->nesaTaFault;
  }
  if (!_data->nesaTaLastError.isEmpty()) ta["last_error"] = _data->nesaTaLastError;

  JsonObject rsg = doc["nesa_rsg1_n"].to<JsonObject>();
  rsg["enabled"] = _cfg->nesaRsg1Enabled;
  rsg["ok"] = _data->nesaRsg1Ok;
  if (_data->nesaRsg1Ok) {
    rsg["raw_adc"] = _data->nesaRsg1Raw;
    rsg["millivolts"] = _data->nesaRsg1MilliVolts;
    rsg["radiation_wm2"] = _data->nesaRsg1RadiationWm2;
    rsg["sensitivity_uv_per_wm2"] = _cfg->nesaRsg1SensitivityUvPerWm2;
  }
  if (!_data->nesaRsg1LastError.isEmpty()) rsg["last_error"] = _data->nesaRsg1LastError;

  JsonObject uv = doc["uv"].to<JsonObject>();
  uv["enabled"] = _cfg->uvEnabled;
  uv["ok"] = _data->uvOk;
  if (_data->uvOk) {
    uv["raw_adc"] = _data->uvRawAdc;
    uv["millivolts"] = _data->uvMilliVolts;
    uv["uv_index"] = _data->uvIndex;
  }

  JsonObject sds = doc["sds011"].to<JsonObject>();
  sds["enabled"] = _cfg->sdsEnabled;
  sds["ok"] = _data->sdsOk;
  sds["state"] = _data->sdsState;
  sds["next_measurement_s"] = _data->sdsNextInSec;
  sds["stage_remaining_s"] = _data->sdsStageRemainingSec;
  sds["samples_collected"] = _data->sdsSamplesCollected;
  sds["attempts"] = _data->sdsAttempts;
  sds["successful_cycles"] = _data->sdsSuccessfulCycles;
  sds["failed_cycles"] = _data->sdsFailedCycles;
  if (!_data->sdsLastError.isEmpty()) sds["last_error"] = _data->sdsLastError;
  if (!isnan(_data->pm25)) sds["pm25_ugm3"] = _data->pm25;
  if (!isnan(_data->pm10)) sds["pm10_ugm3"] = _data->pm10;

  JsonObject as = doc["as3935"].to<JsonObject>();
  as["enabled"] = _cfg->as3935Enabled;
  as["ok"] = _data->as3935Ok;
  as["last_event"] = _data->as3935LastEvent;
  as["lightning_count"] = _data->as3935EventCount;
  as["noise_count"] = _data->as3935NoiseCount;
  as["disturber_count"] = _data->as3935DisturberCount;
  if (_data->as3935DistanceKm >= 0) as["distance_km"] = _data->as3935DistanceKm;
  as["energy"] = _data->as3935Energy;

  _payloadBuffer.remove(0);
  serializeJson(doc, _payloadBuffer);
  String t = topic("telemetry");
  const bool ok = _mqtt->publish(t.c_str(), _payloadBuffer.c_str(), _cfg->mqttRetain);
  registerPublishResult(ok);
  if (ok) _data->lastTelemetryEpoch = epoch;
  return ok;
}
