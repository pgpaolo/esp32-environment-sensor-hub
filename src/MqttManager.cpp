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
    if (cfg.mqttTlsInsecure) {
      _secure.setInsecure();
    } else if (!cfg.mqttCaCert.isEmpty()) {
      _secure.setCACert(cfg.mqttCaCert.c_str());
    }
    _mqtt = new PubSubClient(_secure);
  } else {
    _mqtt = new PubSubClient(_plain);
  }

  _mqtt->setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
  _mqtt->setBufferSize(4096);
}

String MqttManager::topic(const char *suffix) const {
  String t = _cfg->mqttBaseTopic;
  while (t.endsWith("/")) t.remove(t.length() - 1);
  t += "/";
  t += suffix;
  return t;
}

bool MqttManager::ensureConnected() {
  if (!_mqtt || WiFi.status() != WL_CONNECTED || _cfg->mqttHost.isEmpty()) {
    if (_data) _data->mqttConnected = false;
    return false;
  }
  if (_mqtt->connected()) {
    if (_data) {
      _data->mqttConnected = true;
      _data->mqttState = 0;
    }
    return true;
  }

  const uint32_t now = millis();
  const uint32_t retryMs = (uint32_t)max((uint16_t)1, _cfg->mqttReconnectSec) * 1000UL;
  if ((uint32_t)(now - _lastConnectAttemptMs) < retryMs) return false;
  _lastConnectAttemptMs = now;

  String clientId = _cfg->deviceName + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  String willTopic = topic("status");

  bool ok = false;
  if (_cfg->mqttUser.length()) {
    ok = _mqtt->connect(clientId.c_str(), _cfg->mqttUser.c_str(), _cfg->mqttPassword.c_str(),
                        willTopic.c_str(), 0, true, "offline");
  } else {
    ok = _mqtt->connect(clientId.c_str(), willTopic.c_str(), 0, true, "offline");
  }

  if (_data) {
    _data->mqttConnected = ok;
    _data->mqttState = ok ? 0 : _mqtt->state();
    _data->mqttReconnects++;
  }
  if (ok) publishAvailability(true);
  return ok;
}

void MqttManager::loop() {
  if (ensureConnected()) _mqtt->loop();
  if (_data && _mqtt) {
    _data->mqttConnected = _mqtt->connected();
    _data->mqttState = _mqtt->state();
  }
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
  return _mqtt->publish(t.c_str(), online ? "online" : "offline", true);
}

bool MqttManager::publishTelemetry(const char *reason) {
  if (!ensureConnected()) return false;

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

  if (_cfg->relayEnabled) {
    JsonObject relay = doc["relay"].to<JsonObject>();
    relay["enabled"] = true;
    relay["state"] = _data->relayState ? "ON" : "OFF";
  }

  if (_cfg->bh1750Enabled) {
    JsonObject bh = doc["bh1750"].to<JsonObject>();
    bh["ok"] = _data->bh1750Ok;
    if (_data->bh1750Ok) bh["illuminance_lux"] = _data->bh1750Lux;
  }

  if (_cfg->bmeEnabled) {
    JsonObject bme = doc["bme280"].to<JsonObject>();
    bme["ok"] = _data->bmeOk;
    if (_data->bmeOk) {
      bme["temperature_c"] = _data->bmeTempC;
      bme["humidity_pct"] = _data->bmeHumidity;
      bme["pressure_hpa"] = _data->bmePressureHpa;
      bme["dewpoint_c"] = _data->bmeDewPointC;
    }
  }

  if (_cfg->dhtEnabled) {
    JsonObject dht = doc["dht11"].to<JsonObject>();
    dht["ok"] = _data->dhtOk;
    if (_data->dhtOk) {
      dht["temperature_c"] = _data->dhtTempC;
      dht["humidity_pct"] = _data->dhtHumidity;
      dht["dewpoint_c"] = _data->dhtDewPointC;
    }
  }

  if (_cfg->inaEnabled) {
    JsonObject ina = doc["ina219"].to<JsonObject>();
    ina["ok"] = _data->inaOk;
    if (_data->inaOk) {
      ina["bus_voltage_v"] = _data->inaBusVoltageV;
      ina["shunt_voltage_mv"] = _data->inaShuntVoltageMv;
      ina["load_voltage_v"] = _data->inaLoadVoltageV;
      ina["current_ma"] = _data->inaCurrentMa;
      ina["power_mw"] = _data->inaPowerMw;
    }
  }

  if (_cfg->sdsEnabled) {
    JsonObject sds = doc["sds011"].to<JsonObject>();
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
    if (_data->sdsLastSampleEpoch) sds["last_sample_epoch"] = _data->sdsLastSampleEpoch;
  }

  if (_cfg->uvEnabled) {
    JsonObject uv = doc["uv"].to<JsonObject>();
    uv["ok"] = _data->uvOk;
    if (_data->uvOk) {
      uv["raw_adc"] = _data->uvRawAdc;
      uv["millivolts"] = _data->uvMilliVolts;
      uv["uv_index"] = _data->uvIndex;
    }
  }

  if (_cfg->as3935Enabled) {
    JsonObject as = doc["as3935"].to<JsonObject>();
    as["ok"] = _data->as3935Ok;
    as["last_event"] = _data->as3935LastEvent;
    as["lightning_count"] = _data->as3935EventCount;
    as["noise_count"] = _data->as3935NoiseCount;
    as["disturber_count"] = _data->as3935DisturberCount;
    if (_data->as3935DistanceKm >= 0) as["distance_km"] = _data->as3935DistanceKm;
    as["energy"] = _data->as3935Energy;
    if (_data->as3935LastEventEpoch) as["last_event_epoch"] = _data->as3935LastEventEpoch;
  }

  String payload;
  serializeJson(doc, payload);
  String t = topic("telemetry");
  const bool ok = _mqtt->publish(t.c_str(), payload.c_str(), _cfg->mqttRetain);
  if (ok && _data) _data->lastTelemetryEpoch = epoch;
  return ok;
}
