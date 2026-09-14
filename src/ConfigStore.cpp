#include "ConfigStore.h"

bool ConfigStore::load(AppConfig &c) {
  Preferences p;
  if (!p.begin("sensorhub", true)) return false;

  c.deviceName = p.getString("dev", c.deviceName);
  c.wifiSsid = p.getString("wssid", c.wifiSsid);
  c.wifiPassword = p.getString("wpass", c.wifiPassword);
  c.wifiStaticIp = p.getBool("wstatic", c.wifiStaticIp);
  c.wifiIp = p.getString("wip", c.wifiIp);
  c.wifiGateway = p.getString("wgw", c.wifiGateway);
  c.wifiSubnet = p.getString("wsub", c.wifiSubnet);
  c.wifiDns1 = p.getString("wdns1", c.wifiDns1);
  c.wifiDns2 = p.getString("wdns2", c.wifiDns2);
  c.timezone = p.getString("tz", c.timezone);
  c.webUser = p.getString("webu", c.webUser);
  c.webPassword = p.getString("webp", c.webPassword);

  c.mqttHost = p.getString("mqhost", c.mqttHost);
  c.mqttPort = p.getUShort("mqport", c.mqttPort);
  c.mqttUser = p.getString("mquser", c.mqttUser);
  c.mqttPassword = p.getString("mqpass", c.mqttPassword);
  c.mqttBaseTopic = p.getString("mqtopic", c.mqttBaseTopic);
  c.mqttRetain = p.getBool("mqretain", c.mqttRetain);
  c.mqttTls = p.getBool("mqtls", c.mqttTls);
  c.mqttTlsInsecure = p.getBool("mqtlsi", c.mqttTlsInsecure);
  c.mqttCaCert = p.getString("mqca", c.mqttCaCert);
  c.mqttReconnectSec = p.getUShort("mqrecon", c.mqttReconnectSec);

  c.sensorIntervalSec = p.getUInt("sens_s", c.sensorIntervalSec);
  c.telemetryIntervalSec = p.getUInt("tele_s", c.telemetryIntervalSec);
  c.i2cSda = p.getUChar("sda", c.i2cSda);
  c.i2cScl = p.getUChar("scl", c.i2cScl);
  c.statusLedEnabled = p.getBool("led_en", c.statusLedEnabled);
  c.statusLedPin = p.getUChar("led_p", c.statusLedPin);
  c.statusLedInverted = p.getBool("led_inv", c.statusLedInverted);
  c.relayEnabled = p.getBool("rel_en", c.relayEnabled);
  c.relayPin = p.getUChar("rel_p", c.relayPin);
  c.relayInverted = p.getBool("rel_inv", c.relayInverted);

  c.bh1750Enabled = p.getBool("bh_en", c.bh1750Enabled);
  c.bh1750Address = p.getUChar("bh_a", c.bh1750Address);
  c.bh1750OffsetLux = p.getFloat("bh_off", c.bh1750OffsetLux);

  c.bmeEnabled = p.getBool("bme_en", c.bmeEnabled);
  c.bmeAddress = p.getUChar("bme_a", c.bmeAddress);
  c.bmePressureOffsetHpa = p.getFloat("bme_po", c.bmePressureOffsetHpa);
  c.bmeTemperatureOffsetC = p.getFloat("bme_to", c.bmeTemperatureOffsetC);
  c.bmeHumidityOffsetPct = p.getFloat("bme_ho", c.bmeHumidityOffsetPct);

  c.dhtEnabled = p.getBool("dht_en", c.dhtEnabled);
  c.dhtPin = p.getUChar("dht_p", c.dhtPin);
  c.dhtTemperatureOffsetC = p.getFloat("dht_to", c.dhtTemperatureOffsetC);
  c.dhtHumidityOffsetPct = p.getFloat("dht_ho", c.dhtHumidityOffsetPct);

  c.inaEnabled = p.getBool("ina_en", c.inaEnabled);
  c.inaAddress = p.getUChar("ina_a", c.inaAddress);
  c.inaCalibration = static_cast<Ina219Calibration>(p.getUChar("ina_cal", static_cast<uint8_t>(c.inaCalibration)));
  c.inaBusVoltageOffsetV = p.getFloat("ina_vo", c.inaBusVoltageOffsetV);
  c.inaCurrentOffsetMa = p.getFloat("ina_io", c.inaCurrentOffsetMa);

  c.sdsEnabled = p.getBool("sds_en", c.sdsEnabled);
  c.sdsRxPin = p.getUChar("sds_rx", c.sdsRxPin);
  c.sdsTxPin = p.getUChar("sds_tx", c.sdsTxPin);
  c.sdsCycleMinutes = p.getUShort("sds_cm", c.sdsCycleMinutes);
  c.sdsWarmupSec = p.getUShort("sds_w", c.sdsWarmupSec);
  c.sdsSamples = p.getUChar("sds_n", c.sdsSamples);
  c.sdsSampleGapMs = p.getUShort("sds_g", c.sdsSampleGapMs);
  c.sdsMaxAwakeSec = p.getUShort("sds_ma", c.sdsMaxAwakeSec);
  c.sdsFirstCycleDelaySec = p.getUShort("sds_fd", c.sdsFirstCycleDelaySec);

  c.uvEnabled = p.getBool("uv_en", c.uvEnabled);
  c.uvPin = p.getUChar("uv_p", c.uvPin);
  c.uvCalibrationMode = static_cast<UvCalibrationMode>(p.getUChar("uv_mode", static_cast<uint8_t>(c.uvCalibrationMode)));
  c.uvZeroMv = p.getFloat("uv_z", c.uvZeroMv);
  c.uvMvPerIndex = p.getFloat("uv_k", c.uvMvPerIndex);
  c.uvSamples = p.getUChar("uv_n", c.uvSamples);
  c.uvSampleGapUs = p.getUShort("uv_gap", c.uvSampleGapUs);
  c.uvMaxIndex = p.getFloat("uv_max", c.uvMaxIndex);

  c.as3935Enabled = p.getBool("as_en", c.as3935Enabled);
  c.as3935Address = p.getUChar("as_a", c.as3935Address);
  c.as3935IrqPin = p.getUChar("as_irq", c.as3935IrqPin);
  c.as3935Outdoor = p.getBool("as_out", c.as3935Outdoor);
  c.as3935NoiseFloor = p.getUChar("as_nf", c.as3935NoiseFloor);
  c.as3935Watchdog = p.getUChar("as_wd", c.as3935Watchdog);
  c.as3935SpikeRejection = p.getUChar("as_sp", c.as3935SpikeRejection);
  c.as3935LightningThreshold = p.getUChar("as_lt", c.as3935LightningThreshold);
  c.as3935MaskDisturber = p.getBool("as_md", c.as3935MaskDisturber);

  c.configButtonPin = p.getUChar("cfgbtn", c.configButtonPin);
  p.end();
  return true;
}

bool ConfigStore::save(const AppConfig &c) {
  Preferences p;
  if (!p.begin("sensorhub", false)) return false;

  p.putString("dev", c.deviceName);
  p.putString("wssid", c.wifiSsid);
  p.putString("wpass", c.wifiPassword);
  p.putBool("wstatic", c.wifiStaticIp);
  p.putString("wip", c.wifiIp);
  p.putString("wgw", c.wifiGateway);
  p.putString("wsub", c.wifiSubnet);
  p.putString("wdns1", c.wifiDns1);
  p.putString("wdns2", c.wifiDns2);
  p.putString("tz", c.timezone);
  p.putString("webu", c.webUser);
  p.putString("webp", c.webPassword);

  p.putString("mqhost", c.mqttHost);
  p.putUShort("mqport", c.mqttPort);
  p.putString("mquser", c.mqttUser);
  p.putString("mqpass", c.mqttPassword);
  p.putString("mqtopic", c.mqttBaseTopic);
  p.putBool("mqretain", c.mqttRetain);
  p.putBool("mqtls", c.mqttTls);
  p.putBool("mqtlsi", c.mqttTlsInsecure);
  p.putString("mqca", c.mqttCaCert);
  p.putUShort("mqrecon", c.mqttReconnectSec);

  p.putUInt("sens_s", c.sensorIntervalSec);
  p.putUInt("tele_s", c.telemetryIntervalSec);
  p.putUChar("sda", c.i2cSda);
  p.putUChar("scl", c.i2cScl);
  p.putBool("led_en", c.statusLedEnabled);
  p.putUChar("led_p", c.statusLedPin);
  p.putBool("led_inv", c.statusLedInverted);
  p.putBool("rel_en", c.relayEnabled);
  p.putUChar("rel_p", c.relayPin);
  p.putBool("rel_inv", c.relayInverted);

  p.putBool("bh_en", c.bh1750Enabled);
  p.putUChar("bh_a", c.bh1750Address);
  p.putFloat("bh_off", c.bh1750OffsetLux);

  p.putBool("bme_en", c.bmeEnabled);
  p.putUChar("bme_a", c.bmeAddress);
  p.putFloat("bme_po", c.bmePressureOffsetHpa);
  p.putFloat("bme_to", c.bmeTemperatureOffsetC);
  p.putFloat("bme_ho", c.bmeHumidityOffsetPct);

  p.putBool("dht_en", c.dhtEnabled);
  p.putUChar("dht_p", c.dhtPin);
  p.putFloat("dht_to", c.dhtTemperatureOffsetC);
  p.putFloat("dht_ho", c.dhtHumidityOffsetPct);

  p.putBool("ina_en", c.inaEnabled);
  p.putUChar("ina_a", c.inaAddress);
  p.putUChar("ina_cal", static_cast<uint8_t>(c.inaCalibration));
  p.putFloat("ina_vo", c.inaBusVoltageOffsetV);
  p.putFloat("ina_io", c.inaCurrentOffsetMa);

  p.putBool("sds_en", c.sdsEnabled);
  p.putUChar("sds_rx", c.sdsRxPin);
  p.putUChar("sds_tx", c.sdsTxPin);
  p.putUShort("sds_cm", c.sdsCycleMinutes);
  p.putUShort("sds_w", c.sdsWarmupSec);
  p.putUChar("sds_n", c.sdsSamples);
  p.putUShort("sds_g", c.sdsSampleGapMs);
  p.putUShort("sds_ma", c.sdsMaxAwakeSec);
  p.putUShort("sds_fd", c.sdsFirstCycleDelaySec);

  p.putBool("uv_en", c.uvEnabled);
  p.putUChar("uv_p", c.uvPin);
  p.putUChar("uv_mode", static_cast<uint8_t>(c.uvCalibrationMode));
  p.putFloat("uv_z", c.uvZeroMv);
  p.putFloat("uv_k", c.uvMvPerIndex);
  p.putUChar("uv_n", c.uvSamples);
  p.putUShort("uv_gap", c.uvSampleGapUs);
  p.putFloat("uv_max", c.uvMaxIndex);

  p.putBool("as_en", c.as3935Enabled);
  p.putUChar("as_a", c.as3935Address);
  p.putUChar("as_irq", c.as3935IrqPin);
  p.putBool("as_out", c.as3935Outdoor);
  p.putUChar("as_nf", c.as3935NoiseFloor);
  p.putUChar("as_wd", c.as3935Watchdog);
  p.putUChar("as_sp", c.as3935SpikeRejection);
  p.putUChar("as_lt", c.as3935LightningThreshold);
  p.putBool("as_md", c.as3935MaskDisturber);

  p.putUChar("cfgbtn", c.configButtonPin);
  p.end();
  return true;
}

void ConfigStore::clear() {
  Preferences p;
  if (p.begin("sensorhub", false)) {
    p.clear();
    p.end();
  }
}
