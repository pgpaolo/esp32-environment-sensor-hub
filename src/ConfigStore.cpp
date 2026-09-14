#include "ConfigStore.h"

#include <math.h>

namespace {
template <typename T>
bool fixValue(bool bad, T &field, const T &fallback) {
  if (!bad) return false;
  field = fallback;
  return true;
}
}

bool ConfigStore::validGpio(uint8_t pin) {
  return pin <= 39 && !(pin >= 6 && pin <= 11);
}

bool ConfigStore::validOutputGpio(uint8_t pin) {
  return validGpio(pin) && pin < 34;
}

bool ConfigStore::validAdc1Gpio(uint8_t pin) {
  return pin >= 32 && pin <= 39;
}

bool ConfigStore::validate(AppConfig &c) {
  const AppConfig d;
  bool changed = false;

  changed |= fixValue(c.deviceName.isEmpty() || c.deviceName.length() > 31, c.deviceName, d.deviceName);
  changed |= fixValue(c.webUser.isEmpty() || c.webUser.length() > 31, c.webUser, d.webUser);
  changed |= fixValue(c.webPassword.isEmpty() || c.webPassword.length() > 63, c.webPassword, d.webPassword);
  changed |= fixValue(c.mqttPort == 0, c.mqttPort, d.mqttPort);
  changed |= fixValue(c.mqttReconnectSec < 1 || c.mqttReconnectSec > 300, c.mqttReconnectSec, d.mqttReconnectSec);
  changed |= fixValue(c.sensorIntervalSec < 2 || c.sensorIntervalSec > 86400, c.sensorIntervalSec, d.sensorIntervalSec);
  changed |= fixValue(c.telemetryIntervalSec < 5 || c.telemetryIntervalSec > 86400, c.telemetryIntervalSec, d.telemetryIntervalSec);

  changed |= fixValue(!validOutputGpio(c.i2cSda), c.i2cSda, d.i2cSda);
  changed |= fixValue(!validOutputGpio(c.i2cScl), c.i2cScl, d.i2cScl);
  if (c.i2cSda == c.i2cScl) {
    c.i2cSda = d.i2cSda;
    c.i2cScl = d.i2cScl;
    changed = true;
  }

  changed |= fixValue(!validOutputGpio(c.statusLedPin), c.statusLedPin, d.statusLedPin);
  changed |= fixValue(!validOutputGpio(c.relayPin), c.relayPin, d.relayPin);
  changed |= fixValue(!validGpio(c.dhtPin), c.dhtPin, d.dhtPin);
  changed |= fixValue(!validAdc1Gpio(c.uvPin), c.uvPin, d.uvPin);
  changed |= fixValue(!validGpio(c.sdsRxPin), c.sdsRxPin, d.sdsRxPin);
  changed |= fixValue(!validOutputGpio(c.sdsTxPin), c.sdsTxPin, d.sdsTxPin);
  if (c.sdsRxPin == c.sdsTxPin) {
    c.sdsRxPin = d.sdsRxPin;
    c.sdsTxPin = d.sdsTxPin;
    changed = true;
  }
  changed |= fixValue(!validGpio(c.as3935IrqPin), c.as3935IrqPin, d.as3935IrqPin);
  changed |= fixValue(!validOutputGpio(c.nesaTaCsPin), c.nesaTaCsPin, d.nesaTaCsPin);
  changed |= fixValue(!validGpio(c.configButtonPin), c.configButtonPin, d.configButtonPin);

  changed |= fixValue(!(c.bh1750Address == 0x23 || c.bh1750Address == 0x5C), c.bh1750Address, d.bh1750Address);
  changed |= fixValue(!(c.bmeAddress == 0x76 || c.bmeAddress == 0x77), c.bmeAddress, d.bmeAddress);
  changed |= fixValue(c.inaAddress < 0x40 || c.inaAddress > 0x4F, c.inaAddress, d.inaAddress);
  changed |= fixValue(c.as3935Address > 0x03, c.as3935Address, d.as3935Address);
  changed |= fixValue(c.nesaRsg1AdsAddress < 0x48 || c.nesaRsg1AdsAddress > 0x4B, c.nesaRsg1AdsAddress, d.nesaRsg1AdsAddress);

  changed |= fixValue(c.sdsCycleMinutes < 1 || c.sdsCycleMinutes > 1440, c.sdsCycleMinutes, d.sdsCycleMinutes);
  changed |= fixValue(c.sdsWarmupSec < 15 || c.sdsWarmupSec > 180, c.sdsWarmupSec, d.sdsWarmupSec);
  changed |= fixValue(c.sdsSamples < 1 || c.sdsSamples > 30, c.sdsSamples, d.sdsSamples);
  changed |= fixValue(c.sdsSampleGapMs < 250 || c.sdsSampleGapMs > 10000, c.sdsSampleGapMs, d.sdsSampleGapMs);
  changed |= fixValue(c.sdsMaxAwakeSec < 30 || c.sdsMaxAwakeSec > 600, c.sdsMaxAwakeSec, d.sdsMaxAwakeSec);
  changed |= fixValue(c.sdsFirstCycleDelaySec > 600, c.sdsFirstCycleDelaySec, d.sdsFirstCycleDelaySec);

  changed |= fixValue(!isfinite(c.uvZeroMv) || c.uvZeroMv < -1000.0f || c.uvZeroMv > 3000.0f, c.uvZeroMv, d.uvZeroMv);
  changed |= fixValue(!isfinite(c.uvMvPerIndex) || c.uvMvPerIndex <= 0.01f || c.uvMvPerIndex > 5000.0f, c.uvMvPerIndex, d.uvMvPerIndex);
  changed |= fixValue(!isfinite(c.uvMaxIndex) || c.uvMaxIndex <= 0.0f || c.uvMaxIndex > 100.0f, c.uvMaxIndex, d.uvMaxIndex);
  changed |= fixValue(c.uvSamples < 1 || c.uvSamples > 128, c.uvSamples, d.uvSamples);

  changed |= fixValue(c.as3935NoiseFloor > 7, c.as3935NoiseFloor, d.as3935NoiseFloor);
  changed |= fixValue(c.as3935Watchdog > 10, c.as3935Watchdog, d.as3935Watchdog);
  changed |= fixValue(c.as3935SpikeRejection > 15, c.as3935SpikeRejection, d.as3935SpikeRejection);
  if (!(c.as3935LightningThreshold == 1 || c.as3935LightningThreshold == 5 ||
        c.as3935LightningThreshold == 9 || c.as3935LightningThreshold == 16)) {
    c.as3935LightningThreshold = d.as3935LightningThreshold;
    changed = true;
  }

  changed |= fixValue(!isfinite(c.nesaTaRtdNominalOhm) || c.nesaTaRtdNominalOhm < 50.0f || c.nesaTaRtdNominalOhm > 2000.0f,
                      c.nesaTaRtdNominalOhm, d.nesaTaRtdNominalOhm);
  changed |= fixValue(!isfinite(c.nesaTaRefResistorOhm) || c.nesaTaRefResistorOhm < 100.0f || c.nesaTaRefResistorOhm > 10000.0f,
                      c.nesaTaRefResistorOhm, d.nesaTaRefResistorOhm);
  changed |= fixValue(!isfinite(c.nesaRsg1SensitivityUvPerWm2) || c.nesaRsg1SensitivityUvPerWm2 <= 0.0001f || c.nesaRsg1SensitivityUvPerWm2 > 10000.0f,
                      c.nesaRsg1SensitivityUvPerWm2, d.nesaRsg1SensitivityUvPerWm2);
  changed |= fixValue(!isfinite(c.nesaRsg1MaxWm2) || c.nesaRsg1MaxWm2 < 10.0f || c.nesaRsg1MaxWm2 > 10000.0f,
                      c.nesaRsg1MaxWm2, d.nesaRsg1MaxWm2);

  return changed;
}

bool ConfigStore::load(AppConfig &c) {
  Preferences p;
  const bool opened = p.begin("sensorhub", true);
  uint16_t storedSchema = 0;

  if (opened) {
    storedSchema = p.getUShort("cfgver", 0);
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
  }

  const bool corrected = validate(c);
  if (!opened || storedSchema < CURRENT_SCHEMA_VERSION || corrected) save(c);
  return opened;
}

bool ConfigStore::save(const AppConfig &c) {
  Preferences p;
  if (!p.begin("sensorhub", false)) return false;

  p.putUShort("cfgver", CURRENT_SCHEMA_VERSION);
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
