#include "SensorHub.h"
#include <math.h>
#include <time.h>

SensorHub *SensorHub::_instance = nullptr;

SensorHub::SensorHub() : _sdsSerial(Serial2), _sds(Serial2) {}

SensorHub::~SensorHub() {
  if (_dht) delete _dht;
  if (_ina) delete _ina;
  if (_lightning) delete _lightning;
}

void IRAM_ATTR SensorHub::as3935Isr() {
  if (_instance) _instance->_asIrq = true;
}

uint32_t SensorHub::nowEpoch() {
  time_t t = time(nullptr);
  return t > 1700000000 ? (uint32_t)t : 0;
}

bool SensorHub::i2cPresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void SensorHub::begin(AppConfig &cfg, RuntimeData &data) {
  _cfg = &cfg;
  _data = &data;
  _instance = this;

  Wire.begin(cfg.i2cSda, cfg.i2cScl);
  Wire.setTimeOut(80);
  Wire.setClock(100000);

  beginGpio();
  beginBh1750();
  beginBme();
  beginDht();
  beginIna();
  beginSds();
  beginAs3935();
  sampleSlowSensors();
}

void SensorHub::beginGpio() {
  if (_cfg->statusLedEnabled) {
    pinMode(_cfg->statusLedPin, OUTPUT);
    digitalWrite(_cfg->statusLedPin, _cfg->statusLedInverted ? HIGH : LOW);
  }
  if (_cfg->relayEnabled) {
    pinMode(_cfg->relayPin, OUTPUT);
    _data->relayState = false;
    digitalWrite(_cfg->relayPin, _cfg->relayInverted ? HIGH : LOW);
  }
}

void SensorHub::beginBh1750() {
  if (!_cfg->bh1750Enabled) { _data->bh1750Ok = false; return; }
  const uint8_t candidates[] = {_cfg->bh1750Address, 0x23, 0x5C};
  for (uint8_t a : candidates) {
    if (_data->bh1750DetectedAddress != 0xFF && a == _data->bh1750DetectedAddress) continue;
    if (!_bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, a, &Wire)) continue;
    _data->bh1750DetectedAddress = a;
    _data->bh1750Ok = true;
    _data->bh1750LastError = "";
    return;
  }
  _data->bh1750Ok = false;
  _data->bh1750Failures++;
  _data->bh1750LastError = "not_found";
}

bool SensorHub::setRelay(bool on) {
  if (!_cfg || !_data || !_cfg->relayEnabled) return false;
  _data->relayState = on;
  const bool level = _cfg->relayInverted ? !on : on;
  digitalWrite(_cfg->relayPin, level ? HIGH : LOW);
  _immediatePublish = true;
  return true;
}

bool SensorHub::toggleRelay() {
  if (!_data) return false;
  return setRelay(!_data->relayState);
}

void SensorHub::beginBme() {
  if (!_cfg->bmeEnabled) { _data->bmeOk = false; return; }
  const uint8_t candidates[] = {_cfg->bmeAddress, 0x76, 0x77};
  uint8_t tried[3] = {0xFF, 0xFF, 0xFF};
  uint8_t triedCount = 0;
  for (uint8_t a : candidates) {
    bool duplicate = false;
    for (uint8_t i = 0; i < triedCount; ++i) if (tried[i] == a) duplicate = true;
    if (duplicate) continue;
    tried[triedCount++] = a;
    if (_bme.begin(a, &Wire)) {
      _data->bmeDetectedAddress = a;
      _data->bmeOk = true;
      _data->bmeLastError = "";
      return;
    }
  }
  _data->bmeOk = false;
  _data->bmeFailures++;
  _data->bmeLastError = "not_found_0x76_0x77";
}

void SensorHub::beginDht() {
  if (!_cfg->dhtEnabled) { _data->dhtOk = false; return; }
  pinMode(_cfg->dhtPin, INPUT_PULLUP);
  _dht = new DHT(_cfg->dhtPin, DHT11);
  _dht->begin();
  _dhtReadyAtMs = millis() + 2500UL;
  _data->dhtLastError = "warming_up";
}

void SensorHub::beginIna() {
  if (!_cfg->inaEnabled) { _data->inaOk = false; return; }
  _ina = new Adafruit_INA219(_cfg->inaAddress);
  if (!_ina->begin(&Wire)) {
    _data->inaOk = false;
    _data->inaFailures++;
    _data->inaLastError = "not_found";
    return;
  }
  switch (_cfg->inaCalibration) {
    case Ina219Calibration::Range32V1A: _ina->setCalibration_32V_1A(); break;
    case Ina219Calibration::Range16V400mA: _ina->setCalibration_16V_400mA(); break;
    case Ina219Calibration::Range32V2A:
    default: _ina->setCalibration_32V_2A(); break;
  }
  _data->inaDetectedAddress = _cfg->inaAddress;
  _data->inaOk = true;
  _data->inaLastError = "";
}

void SensorHub::beginSds() {
  if (!_cfg->sdsEnabled) {
    _sdsState = SdsState::Disabled;
    _data->sdsState = "disabled";
    return;
  }

  _sdsSerial.begin(9600, SERIAL_8N1, _cfg->sdsRxPin, _cfg->sdsTxPin);
  delay(80);
  _sds.setQueryReportingMode();
  delay(30);
  _sds.sleep();
  _sdsState = SdsState::Sleeping;
  _data->sdsState = "sleeping";
  const uint32_t delaySec = max((uint16_t)1, _cfg->sdsFirstCycleDelaySec);
  _sdsNextCycleMs = millis() + delaySec * 1000UL;
  _data->sdsNextInSec = delaySec;
}

void SensorHub::beginAs3935() {
  if (!_cfg->as3935Enabled) {
    _data->as3935Ok = false;
    _data->as3935LastEvent = "disabled";
    return;
  }

  pinMode(_cfg->as3935IrqPin, INPUT);
  const uint8_t candidates[] = {_cfg->as3935Address, 0x03, 0x02, 0x01, 0x00};
  uint8_t tried[5] = {0xFF,0xFF,0xFF,0xFF,0xFF};
  uint8_t triedCount = 0;

  for (uint8_t a : candidates) {
    bool duplicate = false;
    for (uint8_t i = 0; i < triedCount; ++i) if (tried[i] == a) duplicate = true;
    if (duplicate) continue;
    tried[triedCount++] = a;

    SparkFun_AS3935 *candidate = new SparkFun_AS3935(a);
    if (candidate->begin(Wire)) {
      _lightning = candidate;
      _data->as3935DetectedAddress = a;
      break;
    }
    delete candidate;
  }

  if (!_lightning) {
    _data->as3935Ok = false;
    _data->as3935LastEvent = "init_error";
    _data->as3935LastError = "i2c_not_found";
    return;
  }

  _lightning->setIndoorOutdoor(_cfg->as3935Outdoor ? OUTDOOR : INDOOR);
  _lightning->setNoiseLevel(constrain(_cfg->as3935NoiseFloor, (uint8_t)0, (uint8_t)7));
  _lightning->watchdogThreshold(constrain(_cfg->as3935Watchdog, (uint8_t)0, (uint8_t)10));
  _lightning->spikeRejection(constrain(_cfg->as3935SpikeRejection, (uint8_t)0, (uint8_t)15));

  uint8_t threshold = _cfg->as3935LightningThreshold;
  if (threshold != 1 && threshold != 5 && threshold != 9 && threshold != 16) threshold = 1;
  _lightning->lightningThreshold(threshold);
  _lightning->maskDisturber(_cfg->as3935MaskDisturber);

  _data->as3935Ok = true;
  _data->as3935LastError = "";
  _data->as3935LastEvent = "listening";
  attachInterrupt(digitalPinToInterrupt(_cfg->as3935IrqPin), as3935Isr, RISING);
}

void SensorHub::tick() {
  tickSds();
  tickAs3935();
}

void SensorHub::scheduleNextSds(uint32_t fromMs) {
  const uint32_t cycleMs = (uint32_t)max((uint16_t)1, _cfg->sdsCycleMinutes) * 60UL * 1000UL;
  _sdsNextCycleMs = fromMs + cycleMs;
  _data->sdsNextInSec = cycleMs / 1000UL;
}

void SensorHub::startSdsCycle() {
  if (!_cfg->sdsEnabled || _sdsState == SdsState::Disabled) return;
  _data->sdsLastError = "";
  _data->sdsState = "waking";
  _data->sdsSamplesCollected = 0;
  _data->sdsAttempts = 0;
  _sdsCollected = 0;
  _sdsAttempts = 0;
  _sdsPm25Sum = 0.0f;
  _sdsPm10Sum = 0.0f;

  WorkingStateResult wake = _sds.wakeup();
  (void)wake;
  _sdsAwakeStartMs = millis();
  _sdsState = SdsState::Warming;
  _sdsDeadlineMs = _sdsAwakeStartMs + (uint32_t)max((uint16_t)15, _cfg->sdsWarmupSec) * 1000UL;
  _data->sdsState = "warming";
}

void SensorHub::finishSdsCycle(bool success, const String &error) {
  _sds.sleep();
  _sdsState = SdsState::Sleeping;
  _data->sdsState = "sleeping";
  _data->sdsStageRemainingSec = 0;

  if (success) {
    _data->sdsOk = true;
    _data->sdsLastError = "";
    _data->sdsSuccessfulCycles++;
    _data->sdsLastSampleMs = millis();
    _data->sdsLastSampleEpoch = nowEpoch();
  } else {
    _data->sdsOk = false;
    _data->sdsLastError = error;
    _data->sdsFailedCycles++;
  }

  scheduleNextSds(millis());
  _immediatePublish = true;
}

void SensorHub::tickSds() {
  if (_sdsState == SdsState::Disabled) return;
  const uint32_t now = millis();

  if (_sdsState == SdsState::Sleeping) {
    int32_t remainingMs = (int32_t)(_sdsNextCycleMs - now);
    _data->sdsNextInSec = remainingMs > 0 ? (uint32_t)(remainingMs + 999) / 1000UL : 0;
    _data->sdsStageRemainingSec = 0;
  } else if (_sdsState == SdsState::Warming) {
    int32_t remainingMs = (int32_t)(_sdsDeadlineMs - now);
    _data->sdsStageRemainingSec = remainingMs > 0 ? (uint32_t)(remainingMs + 999) / 1000UL : 0;
    _data->sdsNextInSec = 0;
  } else {
    _data->sdsStageRemainingSec = 0;
    _data->sdsNextInSec = 0;
  }

  if ((_sdsState == SdsState::Warming || _sdsState == SdsState::Sampling) &&
      (uint32_t)(now - _sdsAwakeStartMs) >= (uint32_t)max((uint16_t)30, _cfg->sdsMaxAwakeSec) * 1000UL) {
    finishSdsCycle(false, "measurement_timeout");
    return;
  }

  if (_sdsState == SdsState::Sleeping && (int32_t)(now - _sdsNextCycleMs) >= 0) {
    startSdsCycle();
    return;
  }

  if (_sdsState == SdsState::Warming && (int32_t)(now - _sdsDeadlineMs) >= 0) {
    _sdsCollected = 0;
    _sdsAttempts = 0;
    _sdsPm25Sum = 0.0f;
    _sdsPm10Sum = 0.0f;
    _sdsNextSampleMs = now;
    _sdsState = SdsState::Sampling;
    _data->sdsState = "sampling";
    return;
  }

  if (_sdsState == SdsState::Sampling && (int32_t)(now - _sdsNextSampleMs) >= 0) {
    ++_sdsAttempts;
    _data->sdsAttempts = _sdsAttempts;
    PmResult pm = _sds.queryPm();
    if (pm.isOk()) {
      _sdsPm25Sum += pm.pm25;
      _sdsPm10Sum += pm.pm10;
      ++_sdsCollected;
      _data->sdsSamplesCollected = _sdsCollected;
    }

    if (_sdsCollected >= max((uint8_t)1, _cfg->sdsSamples)) {
      _data->pm25 = _sdsPm25Sum / _sdsCollected;
      _data->pm10 = _sdsPm10Sum / _sdsCollected;
      finishSdsCycle(true);
      return;
    }

    _sdsNextSampleMs = now + max((uint16_t)250, _cfg->sdsSampleGapMs);
  }
}

bool SensorHub::requestSdsMeasurement() {
  if (!_cfg || !_cfg->sdsEnabled || _sdsState == SdsState::Disabled) return false;
  if (_sdsState != SdsState::Sleeping) return false;
  _sdsNextCycleMs = millis();
  _data->sdsNextInSec = 0;
  return true;
}

bool SensorHub::forceSdsSleep(const char *reason) {
  if (!_cfg || !_cfg->sdsEnabled || _sdsState == SdsState::Disabled) return false;
  if (_sdsState == SdsState::Sleeping) return true;
  finishSdsCycle(false, reason ? String(reason) : String("manual_sleep"));
  return true;
}

void SensorHub::tickAs3935() {
  if (!_cfg->as3935Enabled || !_lightning || !_data->as3935Ok) return;
  if (!_asIrq && digitalRead(_cfg->as3935IrqPin) == LOW) return;

  _asIrq = false;
  delay(2);
  const byte event = _lightning->readInterruptReg();

  if (event == NOISE_TO_HIGH) {
    _data->as3935LastEvent = "noise";
    _data->as3935NoiseCount++;
    _data->as3935LastEventEpoch = nowEpoch();
    _immediatePublish = true;
  } else if (event == DISTURBER_DETECT) {
    _data->as3935LastEvent = "disturber";
    _data->as3935DisturberCount++;
    _data->as3935LastEventEpoch = nowEpoch();
    _immediatePublish = true;
  } else if (event == LIGHTNING) {
    _data->as3935LastEvent = "lightning";
    _data->as3935DistanceKm = _lightning->distanceToStorm();
    _data->as3935Energy = _lightning->lightningEnergy();
    _data->as3935EventCount++;
    _data->as3935LastEventEpoch = nowEpoch();
    _immediatePublish = true;
  }
}

float SensorHub::dewPointC(float t, float rh) {
  if (isnan(t) || isnan(rh) || rh <= 0.0f) return NAN;
  const float a = 17.62f;
  const float b = 243.12f;
  const float gamma = log(rh / 100.0f) + (a * t) / (b + t);
  return (b * gamma) / (a - gamma);
}

void SensorHub::sampleDht() {
  if (!_cfg->dhtEnabled || !_dht) { _data->dhtOk = false; return; }
  if ((int32_t)(millis() - _dhtReadyAtMs) < 0) {
    _data->dhtOk = false;
    _data->dhtLastError = "warming_up";
    return;
  }

  float t = _dht->readTemperature(false, true);
  float h = _dht->readHumidity(false);
  if (!isnan(h) && !isnan(t)) {
    t += _cfg->dhtTemperatureOffsetC;
    h = constrain(h + _cfg->dhtHumidityOffsetPct, 0.0f, 100.0f);
    _data->dhtTempC = t;
    _data->dhtHumidity = h;
    _data->dhtDewPointC = dewPointC(t, h);
    _data->dhtOk = true;
    _data->dhtLastSuccessMs = millis();
    _data->dhtLastError = "";
  } else {
    _data->dhtOk = false;
    _data->dhtFailures++;
    _data->dhtLastError = "read_timeout_or_checksum";
    pinMode(_cfg->dhtPin, INPUT_PULLUP);
    _dht->begin();
  }
}

void SensorHub::sampleUv() {
  if (!_cfg->uvEnabled) { _data->uvOk = false; return; }

  analogSetPinAttenuation(_cfg->uvPin, ADC_11db);
  const uint8_t count = constrain(_cfg->uvSamples, (uint8_t)1, (uint8_t)64);
  uint64_t rawSum = 0;
  uint64_t mvSum = 0;
  uint16_t rawMin = 4095, rawMax = 0;

  for (uint8_t i = 0; i < count; ++i) {
    const uint16_t raw = analogRead(_cfg->uvPin);
    const uint32_t mv = analogReadMilliVolts(_cfg->uvPin);
    rawSum += raw;
    mvSum += mv;
    if (raw < rawMin) rawMin = raw;
    if (raw > rawMax) rawMax = raw;
    if (_cfg->uvSampleGapUs) delayMicroseconds(_cfg->uvSampleGapUs);
  }

  _data->uvRawAdc = (uint16_t)(rawSum / count);
  _data->uvMilliVolts = (uint32_t)(mvSum / count);
  _data->uvConsistent = !(_data->uvRawAdc <= 2 && _data->uvMilliVolts >= 25);
  if (!_data->uvConsistent) {
    _data->uvOk = false;
    _data->uvFailures++;
    _data->uvIndex = NAN;
    _data->uvLastError = "adc_inconsistent_or_floating";
    return;
  }

  float zero = _cfg->uvZeroMv;
  float scale = _cfg->uvMvPerIndex;
  if (_cfg->uvCalibrationMode == UvCalibrationMode::Guva100mVPerUvi) scale = 100.0f;
  if (scale <= 0.001f) {
    _data->uvOk = false;
    _data->uvFailures++;
    _data->uvIndex = NAN;
    _data->uvLastError = "invalid_calibration";
    return;
  }

  float uvi = ((float)_data->uvMilliVolts - zero) / scale;
  if (uvi < 0.0f) uvi = 0.0f;
  if (_cfg->uvMaxIndex > 0.0f && uvi > _cfg->uvMaxIndex) uvi = _cfg->uvMaxIndex;
  _data->uvIndex = uvi;
  _data->uvOk = true;
  _data->uvLastError = "";
  (void)rawMin;
  (void)rawMax;
}

void SensorHub::sampleIna() {
  if (!_cfg->inaEnabled || !_ina) { _data->inaOk = false; return; }

  if (!_data->inaOk) {
    if (!_ina->begin(&Wire)) {
      _data->inaFailures++;
      _data->inaLastError = "read_or_init_failed";
      return;
    }
    switch (_cfg->inaCalibration) {
      case Ina219Calibration::Range32V1A: _ina->setCalibration_32V_1A(); break;
      case Ina219Calibration::Range16V400mA: _ina->setCalibration_16V_400mA(); break;
      case Ina219Calibration::Range32V2A:
      default: _ina->setCalibration_32V_2A(); break;
    }
    _data->inaOk = true;
  }

  const float busV = _ina->getBusVoltage_V();
  const float shuntMv = _ina->getShuntVoltage_mV();
  const float currentMa = _ina->getCurrent_mA();
  const float powerMw = _ina->getPower_mW();

  if (!_ina->success() || isnan(busV)) {
    _data->inaOk = false;
    _data->inaFailures++;
    _data->inaLastError = "read_failed";
    return;
  }

  _data->inaBusVoltageV = busV + _cfg->inaBusVoltageOffsetV;
  _data->inaShuntVoltageMv = shuntMv;
  _data->inaLoadVoltageV = _data->inaBusVoltageV + (shuntMv / 1000.0f);
  _data->inaCurrentMa = currentMa + _cfg->inaCurrentOffsetMa;
  _data->inaPowerMw = powerMw;
  _data->inaOk = true;
  _data->inaDetectedAddress = _cfg->inaAddress;
  _data->inaLastError = "";
}

void SensorHub::sampleSlowSensors() {
  if (_cfg->bh1750Enabled) {
    if (!_data->bh1750Ok) beginBh1750();
    if (_data->bh1750Ok) {
      float lux = _bh1750.readLightLevel();
      if (lux >= 0.0f && !isnan(lux)) {
        _data->bh1750Lux = max(0.0f, lux + _cfg->bh1750OffsetLux);
        _data->bh1750LastError = "";
      } else {
        _data->bh1750Ok = false;
        _data->bh1750Failures++;
        _data->bh1750LastError = "read_failed";
      }
    }
  }

  if (_cfg->bmeEnabled) {
    if (!_data->bmeOk) beginBme();
    if (_data->bmeOk) {
      const float t = _bme.readTemperature();
      const float h = _bme.readHumidity();
      const float p = _bme.readPressure();
      if (!isnan(t) && !isnan(h) && !isnan(p)) {
        _data->bmeTempC = t + _cfg->bmeTemperatureOffsetC;
        _data->bmeHumidity = constrain(h + _cfg->bmeHumidityOffsetPct, 0.0f, 100.0f);
        _data->bmePressureHpa = (p / 100.0f) + _cfg->bmePressureOffsetHpa;
        _data->bmeDewPointC = dewPointC(_data->bmeTempC, _data->bmeHumidity);
        _data->bmeLastError = "";
      } else {
        _data->bmeOk = false;
        _data->bmeFailures++;
        _data->bmeLastError = "read_failed";
      }
    }
  }

  sampleDht();
  sampleIna();
  sampleUv();
}

bool SensorHub::takeImmediatePublishFlag() {
  const bool v = _immediatePublish;
  _immediatePublish = false;
  return v;
}

String SensorHub::scanI2c() {
  String result;
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      if (found++) result += " · ";
      char b[7];
      snprintf(b, sizeof(b), "0x%02X", address);
      result += b;
      if (address == _cfg->bh1750Address || address == 0x23 || address == 0x5C) result += " BH1750";
      else if (address == 0x76 || address == 0x77) result += " BME280";
      else if (address == _cfg->inaAddress || address == 0x40) result += " INA219";
      else if (address == _cfg->as3935Address || address == 0x01 || address == 0x02 || address == 0x03) result += " AS3935";
    }
    delay(1);
  }
  if (!found) return "nessun dispositivo rilevato";
  return result;
}
