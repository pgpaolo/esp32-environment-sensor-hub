#include "WebUi.h"
#include "BuildInfo.h"
#include "NesaConfigStore.h"
#include "WebAssets.h"

#include <WiFi.h>
#include <time.h>
#include <esp_system.h>
#include <esp_heap_caps.h>

namespace {
const char *resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external_reset";
    case ESP_RST_SW: return "software_reset";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: return "unknown";
  }
}
}

WebUi::WebUi(AppConfig &cfg, RuntimeData &data, ConfigStore &store, MqttManager &mqtt, SensorHub &sensors)
  : _cfg(cfg), _data(data), _store(store), _mqtt(mqtt), _sensors(sensors), _server(80) {}

uint32_t WebUi::nowEpoch() {
  time_t t = time(nullptr);
  return t > 1700000000 ? (uint32_t)t : 0;
}

bool WebUi::auth() {
  if (_server.authenticate(_cfg.webUser.c_str(), _cfg.webPassword.c_str())) return true;
  _server.requestAuthentication();
  return false;
}

void WebUi::sendJson(JsonDocument &doc) {
  const size_t length = measureJson(doc);
  _server.setContentLength(length);
  _server.send(200, "application/json", "");
  serializeJson(doc, _server.client());
}

void WebUi::handleRoot() {
  if (!auth()) return;
  _server.send_P(200, "text/html; charset=utf-8", WebAssets::ROOT_PAGE);
}

void WebUi::handleApiStatus() {
  if (!auth()) return;

  JsonDocument doc;
  JsonObject sys = doc["system"].to<JsonObject>();
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  float fragmentation = 0.0f;
  if (freeHeap > 0 && largestBlock <= freeHeap) {
    fragmentation = 100.0f * (1.0f - ((float)largestBlock / (float)freeHeap));
    if (fragmentation < 0.0f) fragmentation = 0.0f;
    if (fragmentation > 100.0f) fragmentation = 100.0f;
  }

  sys["wifi"] = WiFi.status() == WL_CONNECTED;
  sys["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  sys["rssi_dbm"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  sys["mqtt"] = _mqtt.connected();
  sys["mqtt_state"] = _mqtt.state();
  sys["uptime_s"] = millis() / 1000UL;
  sys["free_heap"] = freeHeap;
  sys["min_free_heap"] = ESP.getMinFreeHeap();
  sys["largest_free_block"] = largestBlock;
  sys["heap_fragmentation_pct"] = fragmentation;
  sys["boot_count"] = _data.bootCount;
  sys["firmware"] = FW_VERSION;
  sys["config_schema"] = ConfigStore::schemaVersion();
  sys["chip_model"] = ESP.getChipModel();
  sys["chip_revision"] = ESP.getChipRevision();
  sys["flash_size"] = ESP.getFlashChipSize();
  sys["reset_reason"] = resetReasonText(esp_reset_reason());

  JsonObject mq = doc["mqtt"].to<JsonObject>();
  mq["connected"] = _data.mqttConnected;
  mq["state"] = _data.mqttState;
  mq["last_state"] = _data.mqttLastState;
  mq["connect_attempts"] = _data.mqttConnectAttempts;
  mq["connect_success"] = _data.mqttConnectSuccess;
  mq["disconnects"] = _data.mqttDisconnects;
  mq["publish_ok"] = _data.mqttPublishOk;
  mq["publish_failed"] = _data.mqttPublishFailed;
  mq["last_connect_epoch"] = _data.mqttLastConnectEpoch;
  mq["last_publish_epoch"] = _data.mqttLastPublishEpoch;
  mq["last_disconnect_epoch"] = _data.mqttLastDisconnectEpoch;
  mq["backoff_s"] = _data.mqttCurrentBackoffSec;

  JsonObject pins = doc["pins"].to<JsonObject>();
  pins["i2c_sda"] = _cfg.i2cSda;
  pins["i2c_scl"] = _cfg.i2cScl;
  pins["dht_data"] = _cfg.dhtPin;
  pins["uv_adc"] = _cfg.uvPin;
  pins["sds_rx"] = _cfg.sdsRxPin;
  pins["sds_tx"] = _cfg.sdsTxPin;
  pins["as3935_irq"] = _cfg.as3935IrqPin;
  pins["nesa_ta_cs"] = _cfg.nesaTaCsPin;
  pins["spi_sck"] = 18;
  pins["spi_miso"] = 19;
  pins["spi_mosi"] = 23;
  pins["relay"] = _cfg.relayPin;
  pins["status_led"] = _cfg.statusLedPin;
  pins["config_button"] = _cfg.configButtonPin;

  JsonObject relay = doc["relay"].to<JsonObject>();
  relay["enabled"] = _cfg.relayEnabled;
  relay["state"] = _data.relayState;

  JsonObject bh = doc["bh1750"].to<JsonObject>();
  bh["enabled"] = _cfg.bh1750Enabled;
  bh["ok"] = _data.bh1750Ok;
  bh["illuminance_lux"] = _data.bh1750Lux;
  bh["configured_address"] = _cfg.bh1750Address;
  bh["detected_address"] = _data.bh1750DetectedAddress;
  bh["failures"] = _data.bh1750Failures;
  bh["last_error"] = _data.bh1750LastError;

  JsonObject bme = doc["bme280"].to<JsonObject>();
  bme["enabled"] = _cfg.bmeEnabled;
  bme["ok"] = _data.bmeOk;
  bme["temperature_c"] = _data.bmeTempC;
  bme["humidity_pct"] = _data.bmeHumidity;
  bme["pressure_hpa"] = _data.bmePressureHpa;
  bme["dewpoint_c"] = _data.bmeDewPointC;
  bme["configured_address"] = _cfg.bmeAddress;
  bme["detected_address"] = _data.bmeDetectedAddress;
  bme["failures"] = _data.bmeFailures;
  bme["last_error"] = _data.bmeLastError;

  JsonObject dht = doc["dht11"].to<JsonObject>();
  dht["enabled"] = _cfg.dhtEnabled;
  dht["ok"] = _data.dhtOk;
  dht["pin"] = _cfg.dhtPin;
  dht["temperature_c"] = _data.dhtTempC;
  dht["humidity_pct"] = _data.dhtHumidity;
  dht["dewpoint_c"] = _data.dhtDewPointC;
  dht["failures"] = _data.dhtFailures;
  dht["last_error"] = _data.dhtLastError;

  JsonObject ina = doc["ina219"].to<JsonObject>();
  ina["enabled"] = _cfg.inaEnabled;
  ina["ok"] = _data.inaOk;
  ina["bus_voltage_v"] = _data.inaBusVoltageV;
  ina["shunt_voltage_mv"] = _data.inaShuntVoltageMv;
  ina["load_voltage_v"] = _data.inaLoadVoltageV;
  ina["current_ma"] = _data.inaCurrentMa;
  ina["power_mw"] = _data.inaPowerMw;
  ina["configured_address"] = _cfg.inaAddress;
  ina["detected_address"] = _data.inaDetectedAddress;
  ina["failures"] = _data.inaFailures;
  ina["last_error"] = _data.inaLastError;

  JsonObject uv = doc["uv"].to<JsonObject>();
  uv["enabled"] = _cfg.uvEnabled;
  uv["ok"] = _data.uvOk;
  uv["raw_adc"] = _data.uvRawAdc;
  uv["millivolts"] = _data.uvMilliVolts;
  uv["uv_index"] = _data.uvIndex;
  uv["failures"] = _data.uvFailures;
  uv["last_error"] = _data.uvLastError;

  JsonObject ta = doc["nesa_ta_n"].to<JsonObject>();
  ta["enabled"] = _cfg.nesaTaEnabled;
  ta["ok"] = _data.nesaTaOk;
  ta["temperature_c"] = _data.nesaTaTemperatureC;
  ta["resistance_ohm"] = _data.nesaTaResistanceOhm;
  ta["fault"] = _data.nesaTaFault;
  ta["failures"] = _data.nesaTaFailures;
  ta["last_error"] = _data.nesaTaLastError;
  ta["cs_pin"] = _cfg.nesaTaCsPin;

  JsonObject rsg = doc["nesa_rsg1_n"].to<JsonObject>();
  rsg["enabled"] = _cfg.nesaRsg1Enabled;
  rsg["ok"] = _data.nesaRsg1Ok;
  rsg["raw_adc"] = _data.nesaRsg1Raw;
  rsg["millivolts"] = _data.nesaRsg1MilliVolts;
  rsg["radiation_wm2"] = _data.nesaRsg1RadiationWm2;
  rsg["configured_address"] = _cfg.nesaRsg1AdsAddress;
  rsg["detected_address"] = _data.nesaRsg1DetectedAddress;
  rsg["sensitivity_uv_per_wm2"] = _cfg.nesaRsg1SensitivityUvPerWm2;
  rsg["failures"] = _data.nesaRsg1Failures;
  rsg["last_error"] = _data.nesaRsg1LastError;

  JsonObject sds = doc["sds011"].to<JsonObject>();
  sds["enabled"] = _cfg.sdsEnabled;
  sds["ok"] = _data.sdsOk;
  sds["state"] = _data.sdsState;
  if (!isnan(_data.pm25)) sds["pm25_ugm3"] = _data.pm25;
  if (!isnan(_data.pm10)) sds["pm10_ugm3"] = _data.pm10;
  sds["next_measurement_s"] = _data.sdsNextInSec;
  sds["stage_remaining_s"] = _data.sdsStageRemainingSec;
  sds["samples_collected"] = _data.sdsSamplesCollected;
  sds["target_samples"] = _cfg.sdsSamples;
  sds["successful_cycles"] = _data.sdsSuccessfulCycles;
  sds["failed_cycles"] = _data.sdsFailedCycles;
  sds["last_error"] = _data.sdsLastError;

  JsonObject as = doc["as3935"].to<JsonObject>();
  as["enabled"] = _cfg.as3935Enabled;
  as["ok"] = _data.as3935Ok;
  as["last_event"] = _data.as3935LastEvent;
  if (_data.as3935DistanceKm >= 0) as["distance_km"] = _data.as3935DistanceKm;
  as["energy"] = _data.as3935Energy;
  as["lightning_count"] = _data.as3935EventCount;
  as["noise_count"] = _data.as3935NoiseCount;
  as["disturber_count"] = _data.as3935DisturberCount;
  as["configured_address"] = _cfg.as3935Address;
  as["detected_address"] = _data.as3935DetectedAddress;
  as["last_error"] = _data.as3935LastError;

  sendJson(doc);
}

void WebUi::handleApiConfig() {
  if (!auth()) return;

  JsonDocument doc;
  doc["firmware"] = FW_VERSION;
  doc["schema"] = ConfigStore::schemaVersion();

  doc["ssid"] = _cfg.wifiSsid;
  doc["dev"] = _cfg.deviceName;
  doc["tz"] = _cfg.timezone;
  doc["wstatic"] = _cfg.wifiStaticIp;
  doc["wip"] = _cfg.wifiIp;
  doc["wgw"] = _cfg.wifiGateway;
  doc["wsub"] = _cfg.wifiSubnet;
  doc["wdns1"] = _cfg.wifiDns1;
  doc["wdns2"] = _cfg.wifiDns2;
  doc["wifi_password_set"] = !_cfg.wifiPassword.isEmpty();

  doc["mqhost"] = _cfg.mqttHost;
  doc["mqport"] = _cfg.mqttPort;
  doc["mquser"] = _cfg.mqttUser;
  doc["mqtopic"] = _cfg.mqttBaseTopic;
  doc["mqretain"] = _cfg.mqttRetain;
  doc["mqtls"] = _cfg.mqttTls;
  doc["mqtlsi"] = _cfg.mqttTlsInsecure;
  doc["mqrecon"] = _cfg.mqttReconnectSec;
  doc["mqtt_password_set"] = !_cfg.mqttPassword.isEmpty();
  doc["mqtt_ca_set"] = !_cfg.mqttCaCert.isEmpty();

  doc["bh_en"] = _cfg.bh1750Enabled;
  doc["bme_en"] = _cfg.bmeEnabled;
  doc["dht_en"] = _cfg.dhtEnabled;
  doc["uv_en"] = _cfg.uvEnabled;
  doc["ina_en"] = _cfg.inaEnabled;
  doc["sds_en"] = _cfg.sdsEnabled;
  doc["as_en"] = _cfg.as3935Enabled;
  doc["nta_en"] = _cfg.nesaTaEnabled;
  doc["nrg_en"] = _cfg.nesaRsg1Enabled;

  doc["bme_a"] = String(_cfg.bmeAddress, HEX);
  doc["bh_a"] = String(_cfg.bh1750Address, HEX);
  doc["bme_to"] = _cfg.bmeTemperatureOffsetC;
  doc["bme_po"] = _cfg.bmePressureOffsetHpa;
  doc["bme_ho"] = _cfg.bmeHumidityOffsetPct;
  doc["bh_off"] = _cfg.bh1750OffsetLux;
  doc["dht_p"] = _cfg.dhtPin;
  doc["dht_to"] = _cfg.dhtTemperatureOffsetC;
  doc["dht_ho"] = _cfg.dhtHumidityOffsetPct;
  doc["uv_p"] = _cfg.uvPin;
  doc["uv_z"] = _cfg.uvZeroMv;
  doc["uv_k"] = _cfg.uvMvPerIndex;
  doc["uv_max"] = _cfg.uvMaxIndex;
  doc["ina_a"] = String(_cfg.inaAddress, HEX);
  doc["ina_vo"] = _cfg.inaBusVoltageOffsetV;
  doc["ina_io"] = _cfg.inaCurrentOffsetMa;

  doc["sds_rx"] = _cfg.sdsRxPin;
  doc["sds_tx"] = _cfg.sdsTxPin;
  doc["sds_cm"] = _cfg.sdsCycleMinutes;
  doc["sds_w"] = _cfg.sdsWarmupSec;
  doc["sds_n"] = _cfg.sdsSamples;
  doc["sds_g"] = _cfg.sdsSampleGapMs;
  doc["sds_ma"] = _cfg.sdsMaxAwakeSec;

  doc["as_a"] = String(_cfg.as3935Address, HEX);
  doc["as_irq"] = _cfg.as3935IrqPin;
  doc["as_out"] = _cfg.as3935Outdoor;
  doc["as_nf"] = _cfg.as3935NoiseFloor;
  doc["as_wd"] = _cfg.as3935Watchdog;
  doc["as_sp"] = _cfg.as3935SpikeRejection;
  doc["as_lt"] = _cfg.as3935LightningThreshold;
  doc["as_md"] = _cfg.as3935MaskDisturber;

  doc["nta_cs"] = _cfg.nesaTaCsPin;
  doc["nta_rtd"] = _cfg.nesaTaRtdNominalOhm;
  doc["nta_ref"] = _cfg.nesaTaRefResistorOhm;
  doc["nta_off"] = _cfg.nesaTaTemperatureOffsetC;
  doc["nrg_a"] = String(_cfg.nesaRsg1AdsAddress, HEX);
  doc["nrg_s"] = _cfg.nesaRsg1SensitivityUvPerWm2;
  doc["nrg_off"] = _cfg.nesaRsg1OffsetUv;
  doc["nrg_max"] = _cfg.nesaRsg1MaxWm2;
  doc["nrg_cl"] = _cfg.nesaRsg1ClampNegative;

  doc["sens_s"] = _cfg.sensorIntervalSec;
  doc["tele_s"] = _cfg.telemetryIntervalSec;
  doc["sda"] = _cfg.i2cSda;
  doc["scl"] = _cfg.i2cScl;
  doc["rel_en"] = _cfg.relayEnabled;
  doc["rel_p"] = _cfg.relayPin;
  doc["rel_inv"] = _cfg.relayInverted;
  doc["led_en"] = _cfg.statusLedEnabled;
  doc["led_p"] = _cfg.statusLedPin;
  doc["led_inv"] = _cfg.statusLedInverted;
  doc["cfgbtn"] = _cfg.configButtonPin;
  doc["webu"] = _cfg.webUser;
  doc["web_password_set"] = !_cfg.webPassword.isEmpty();

  // Passwords and the stored CA text are deliberately never returned.
  sendJson(doc);
}

uint8_t WebUi::parseHexByte(const String &value, uint8_t fallback) {
  if (!value.length()) return fallback;
  char *end = nullptr;
  long v = strtol(value.c_str(), &end, 16);
  return (end && *end == 0 && v >= 0 && v <= 255) ? (uint8_t)v : fallback;
}

void WebUi::handleConfig() {
  if (!auth()) return;
  _server.send_P(200, "text/html; charset=utf-8", WebAssets::CONFIG_PAGE);
}

void WebUi::handleSave() {
  if (!auth()) return;

  auto a = [this](const char *name, const String &fallback) {
    return _server.hasArg(name) ? _server.arg(name) : fallback;
  };

  _cfg.wifiSsid = a("ssid", _cfg.wifiSsid);
  _cfg.deviceName = a("dev", _cfg.deviceName);
  _cfg.timezone = a("tz", _cfg.timezone);
  _cfg.wifiStaticIp = _server.hasArg("wstatic");
  _cfg.wifiIp = a("wip", _cfg.wifiIp);
  _cfg.wifiGateway = a("wgw", _cfg.wifiGateway);
  _cfg.wifiSubnet = a("wsub", _cfg.wifiSubnet);
  _cfg.wifiDns1 = a("wdns1", _cfg.wifiDns1);
  _cfg.wifiDns2 = a("wdns2", _cfg.wifiDns2);
  if (_server.hasArg("wpass_clear")) _cfg.wifiPassword = "";
  else if (_server.hasArg("wpass") && !_server.arg("wpass").isEmpty()) _cfg.wifiPassword = _server.arg("wpass");

  _cfg.mqttHost = a("mqhost", _cfg.mqttHost);
  _cfg.mqttPort = (uint16_t)constrain(a("mqport", String(_cfg.mqttPort)).toInt(), 1L, 65535L);
  _cfg.mqttUser = a("mquser", _cfg.mqttUser);
  _cfg.mqttBaseTopic = a("mqtopic", _cfg.mqttBaseTopic);
  _cfg.mqttRetain = _server.hasArg("mqretain");
  _cfg.mqttTls = _server.hasArg("mqtls");
  _cfg.mqttTlsInsecure = _server.hasArg("mqtlsi");
  _cfg.mqttReconnectSec = (uint16_t)constrain(a("mqrecon", String(_cfg.mqttReconnectSec)).toInt(), 1L, 300L);
  if (_server.hasArg("mqpass_clear")) _cfg.mqttPassword = "";
  else if (_server.hasArg("mqpass") && !_server.arg("mqpass").isEmpty()) _cfg.mqttPassword = _server.arg("mqpass");
  if (_server.hasArg("mqca_clear")) _cfg.mqttCaCert = "";
  else if (_server.hasArg("mqca") && !_server.arg("mqca").isEmpty()) _cfg.mqttCaCert = _server.arg("mqca");

  _cfg.bh1750Enabled = _server.hasArg("bh_en");
  _cfg.bmeEnabled = _server.hasArg("bme_en");
  _cfg.dhtEnabled = _server.hasArg("dht_en");
  _cfg.uvEnabled = _server.hasArg("uv_en");
  _cfg.inaEnabled = _server.hasArg("ina_en");
  _cfg.sdsEnabled = _server.hasArg("sds_en");
  _cfg.as3935Enabled = _server.hasArg("as_en");
  _cfg.nesaTaEnabled = _server.hasArg("nta_en");
  _cfg.nesaRsg1Enabled = _server.hasArg("nrg_en");

  _cfg.bmeAddress = parseHexByte(a("bme_a", String(_cfg.bmeAddress, HEX)), _cfg.bmeAddress);
  _cfg.bh1750Address = parseHexByte(a("bh_a", String(_cfg.bh1750Address, HEX)), _cfg.bh1750Address);
  _cfg.bmeTemperatureOffsetC = a("bme_to", String(_cfg.bmeTemperatureOffsetC)).toFloat();
  _cfg.bmePressureOffsetHpa = a("bme_po", String(_cfg.bmePressureOffsetHpa)).toFloat();
  _cfg.bmeHumidityOffsetPct = a("bme_ho", String(_cfg.bmeHumidityOffsetPct)).toFloat();
  _cfg.bh1750OffsetLux = a("bh_off", String(_cfg.bh1750OffsetLux)).toFloat();

  _cfg.dhtPin = (uint8_t)a("dht_p", String(_cfg.dhtPin)).toInt();
  _cfg.dhtTemperatureOffsetC = a("dht_to", String(_cfg.dhtTemperatureOffsetC)).toFloat();
  _cfg.dhtHumidityOffsetPct = a("dht_ho", String(_cfg.dhtHumidityOffsetPct)).toFloat();
  _cfg.uvPin = (uint8_t)a("uv_p", String(_cfg.uvPin)).toInt();
  _cfg.uvZeroMv = a("uv_z", String(_cfg.uvZeroMv)).toFloat();
  _cfg.uvMvPerIndex = a("uv_k", String(_cfg.uvMvPerIndex)).toFloat();
  _cfg.uvMaxIndex = a("uv_max", String(_cfg.uvMaxIndex)).toFloat();

  _cfg.inaAddress = parseHexByte(a("ina_a", String(_cfg.inaAddress, HEX)), _cfg.inaAddress);
  _cfg.inaBusVoltageOffsetV = a("ina_vo", String(_cfg.inaBusVoltageOffsetV)).toFloat();
  _cfg.inaCurrentOffsetMa = a("ina_io", String(_cfg.inaCurrentOffsetMa)).toFloat();

  _cfg.sdsRxPin = (uint8_t)a("sds_rx", String(_cfg.sdsRxPin)).toInt();
  _cfg.sdsTxPin = (uint8_t)a("sds_tx", String(_cfg.sdsTxPin)).toInt();
  _cfg.sdsCycleMinutes = (uint16_t)constrain(a("sds_cm", String(_cfg.sdsCycleMinutes)).toInt(), 1L, 1440L);
  _cfg.sdsWarmupSec = (uint16_t)constrain(a("sds_w", String(_cfg.sdsWarmupSec)).toInt(), 15L, 180L);
  _cfg.sdsSamples = (uint8_t)constrain(a("sds_n", String(_cfg.sdsSamples)).toInt(), 1L, 30L);
  _cfg.sdsSampleGapMs = (uint16_t)constrain(a("sds_g", String(_cfg.sdsSampleGapMs)).toInt(), 250L, 10000L);
  _cfg.sdsMaxAwakeSec = (uint16_t)constrain(a("sds_ma", String(_cfg.sdsMaxAwakeSec)).toInt(), 30L, 600L);

  _cfg.as3935Address = parseHexByte(a("as_a", String(_cfg.as3935Address, HEX)), _cfg.as3935Address);
  _cfg.as3935IrqPin = (uint8_t)a("as_irq", String(_cfg.as3935IrqPin)).toInt();
  _cfg.as3935Outdoor = _server.hasArg("as_out");
  _cfg.as3935NoiseFloor = (uint8_t)constrain(a("as_nf", String(_cfg.as3935NoiseFloor)).toInt(), 0L, 7L);
  _cfg.as3935Watchdog = (uint8_t)constrain(a("as_wd", String(_cfg.as3935Watchdog)).toInt(), 0L, 10L);
  _cfg.as3935SpikeRejection = (uint8_t)constrain(a("as_sp", String(_cfg.as3935SpikeRejection)).toInt(), 0L, 15L);
  _cfg.as3935LightningThreshold = (uint8_t)a("as_lt", String(_cfg.as3935LightningThreshold)).toInt();
  _cfg.as3935MaskDisturber = _server.hasArg("as_md");

  _cfg.nesaTaCsPin = (uint8_t)a("nta_cs", String(_cfg.nesaTaCsPin)).toInt();
  _cfg.nesaTaRtdNominalOhm = a("nta_rtd", String(_cfg.nesaTaRtdNominalOhm)).toFloat();
  _cfg.nesaTaRefResistorOhm = a("nta_ref", String(_cfg.nesaTaRefResistorOhm)).toFloat();
  _cfg.nesaTaTemperatureOffsetC = a("nta_off", String(_cfg.nesaTaTemperatureOffsetC)).toFloat();

  _cfg.nesaRsg1AdsAddress = parseHexByte(a("nrg_a", String(_cfg.nesaRsg1AdsAddress, HEX)), _cfg.nesaRsg1AdsAddress);
  _cfg.nesaRsg1SensitivityUvPerWm2 = a("nrg_s", String(_cfg.nesaRsg1SensitivityUvPerWm2)).toFloat();
  _cfg.nesaRsg1OffsetUv = a("nrg_off", String(_cfg.nesaRsg1OffsetUv)).toFloat();
  _cfg.nesaRsg1MaxWm2 = a("nrg_max", String(_cfg.nesaRsg1MaxWm2)).toFloat();
  _cfg.nesaRsg1ClampNegative = _server.hasArg("nrg_cl");

  _cfg.sensorIntervalSec = (uint32_t)constrain(a("sens_s", String(_cfg.sensorIntervalSec)).toInt(), 2L, 86400L);
  _cfg.telemetryIntervalSec = (uint32_t)constrain(a("tele_s", String(_cfg.telemetryIntervalSec)).toInt(), 5L, 86400L);
  _cfg.i2cSda = (uint8_t)a("sda", String(_cfg.i2cSda)).toInt();
  _cfg.i2cScl = (uint8_t)a("scl", String(_cfg.i2cScl)).toInt();
  _cfg.relayEnabled = _server.hasArg("rel_en");
  _cfg.relayPin = (uint8_t)a("rel_p", String(_cfg.relayPin)).toInt();
  _cfg.relayInverted = _server.hasArg("rel_inv");
  _cfg.statusLedEnabled = _server.hasArg("led_en");
  _cfg.statusLedPin = (uint8_t)a("led_p", String(_cfg.statusLedPin)).toInt();
  _cfg.statusLedInverted = _server.hasArg("led_inv");
  _cfg.configButtonPin = (uint8_t)a("cfgbtn", String(_cfg.configButtonPin)).toInt();

  if (_server.hasArg("web_reset")) {
    _cfg.webUser = "admin";
    _cfg.webPassword = "admin";
  } else {
    _cfg.webUser = a("webu", _cfg.webUser);
    if (_server.hasArg("webp") && !_server.arg("webp").isEmpty()) _cfg.webPassword = _server.arg("webp");
  }

  ConfigStore::validate(_cfg);
  _store.save(_cfg);
  saveNesaConfig(_cfg);

  _server.send_P(200, "text/html; charset=utf-8", WebAssets::SAVED_PAGE);
  delay(600);
  ESP.restart();
}

void WebUi::handleFactory() {
  if (!auth()) return;
  _store.clear();
  clearNesaConfig();
  _server.send(200, "text/plain", "Configurazione cancellata. Al riavvio Web user/password tornano admin/admin.");
  delay(500);
  ESP.restart();
}

void WebUi::setupOta() {
  _server.on("/update", HTTP_GET, [this]() {
    if (!auth()) return;
    _server.send_P(200, "text/html; charset=utf-8", WebAssets::OTA_PAGE);
  });

  _server.on("/update", HTTP_POST,
    [this]() {
      const bool ok = !Update.hasError();
      _server.send(200, "text/plain", ok ? "OK - rebooting" : "UPDATE FAILED");
      if (ok) {
        delay(500);
        ESP.restart();
      }
    },
    [this]() {
      HTTPUpload &u = _server.upload();
      if (u.status == UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
      else if (u.status == UPLOAD_FILE_WRITE) Update.write(u.buf, u.currentSize);
      else if (u.status == UPLOAD_FILE_END) Update.end(true);
    });
}

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
  _server.on("/api/config", HTTP_GET, [this]() { handleApiConfig(); });
  _server.on("/api/i2c", HTTP_GET, [this]() {
    if (!auth()) return;
    _server.send(200, "text/plain", _sensors.scanI2c());
  });
  _server.on("/api/relay/toggle", HTTP_POST, [this]() {
    if (!auth()) return;
    _sensors.toggleRelay();
    _server.send(200, "text/plain", "OK");
  });
  _server.on("/api/sds/measure", HTTP_POST, [this]() {
    if (!auth()) return;
    _server.send(_sensors.requestSdsMeasurement() ? 200 : 409, "text/plain", "OK");
  });
  _server.on("/api/sds/sleep", HTTP_POST, [this]() {
    if (!auth()) return;
    _server.send(_sensors.forceSdsSleep() ? 200 : 409, "text/plain", "OK");
  });
  _server.on("/config", HTTP_GET, [this]() { handleConfig(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.on("/factory", HTTP_GET, [this]() { handleFactory(); });
  setupOta();
  _server.begin();
}

void WebUi::loop() {
  _server.handleClient();
}
