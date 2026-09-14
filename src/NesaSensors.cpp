#include "SensorHub.h"
#include <math.h>

void SensorHub::beginNesaSensors() {
  beginNesaTa();
  beginNesaRsg1();
  sampleNesaSensors();
}

void SensorHub::beginNesaTa() {
  if (!_cfg || !_data || !_cfg->nesaTaEnabled) {
    if (_data) _data->nesaTaOk = false;
    return;
  }

  if (_nesaTa) {
    delete _nesaTa;
    _nesaTa = nullptr;
  }

  _nesaTa = new Adafruit_MAX31865(_cfg->nesaTaCsPin, &SPI);
  if (!_nesaTa->begin(MAX31865_4WIRE)) {
    _data->nesaTaOk = false;
    _data->nesaTaFailures++;
    _data->nesaTaLastError = "max31865_init_failed";
    return;
  }

  _nesaTa->clearFault();
  _data->nesaTaLastError = "";
}

void SensorHub::sampleNesaTa() {
  if (!_cfg || !_data || !_cfg->nesaTaEnabled || !_nesaTa) {
    if (_data) _data->nesaTaOk = false;
    return;
  }

  const uint8_t fault = _nesaTa->readFault();
  if (fault != 0) {
    _data->nesaTaOk = false;
    _data->nesaTaFault = fault;
    _data->nesaTaFailures++;
    _data->nesaTaLastError = "rtd_fault_0x" + String(fault, HEX);
    _nesaTa->clearFault();
    return;
  }

  const uint16_t rtd = _nesaTa->readRTD();
  const float resistance = ((float)rtd * _cfg->nesaTaRefResistorOhm) / 32768.0f;
  const float temperature = _nesaTa->temperature(_cfg->nesaTaRtdNominalOhm,
                                                  _cfg->nesaTaRefResistorOhm) +
                            _cfg->nesaTaTemperatureOffsetC;

  if (isnan(temperature) || temperature < -100.0f || temperature > 100.0f ||
      resistance < 40.0f || resistance > 200.0f) {
    _data->nesaTaOk = false;
    _data->nesaTaFailures++;
    _data->nesaTaLastError = "implausible_rtd_reading";
    return;
  }

  _data->nesaTaTemperatureC = temperature;
  _data->nesaTaResistanceOhm = resistance;
  _data->nesaTaFault = 0;
  _data->nesaTaOk = true;
  _data->nesaTaLastError = "";
}

void SensorHub::beginNesaRsg1() {
  if (!_cfg || !_data || !_cfg->nesaRsg1Enabled) {
    if (_data) _data->nesaRsg1Ok = false;
    return;
  }

  if (_nesaRsg1) {
    delete _nesaRsg1;
    _nesaRsg1 = nullptr;
  }

  _nesaRsg1 = new Adafruit_ADS1115();
  if (!_nesaRsg1->begin(_cfg->nesaRsg1AdsAddress, &Wire)) {
    _data->nesaRsg1Ok = false;
    _data->nesaRsg1Failures++;
    _data->nesaRsg1LastError = "ads1115_not_found";
    return;
  }

  // ±0.256 V full-scale: 7.8125 uV/bit on ADS1115, ideal for RSG1-N (~0..20 mV).
  _nesaRsg1->setGain(GAIN_SIXTEEN);
  _nesaRsg1->setDataRate(RATE_ADS1115_128SPS);
  _data->nesaRsg1DetectedAddress = _cfg->nesaRsg1AdsAddress;
  _data->nesaRsg1LastError = "";
}

void SensorHub::sampleNesaRsg1() {
  if (!_cfg || !_data || !_cfg->nesaRsg1Enabled || !_nesaRsg1) {
    if (_data) _data->nesaRsg1Ok = false;
    return;
  }

  if (_cfg->nesaRsg1SensitivityUvPerWm2 <= 0.001f) {
    _data->nesaRsg1Ok = false;
    _data->nesaRsg1Failures++;
    _data->nesaRsg1LastError = "invalid_sensitivity";
    return;
  }

  const int16_t raw = _nesaRsg1->readADC_Differential_0_1();
  const float volts = _nesaRsg1->computeVolts(raw);
  const float microvolts = volts * 1000000.0f;
  float radiation = (microvolts - _cfg->nesaRsg1OffsetUv) /
                    _cfg->nesaRsg1SensitivityUvPerWm2;

  if (_cfg->nesaRsg1ClampNegative && radiation < 0.0f) radiation = 0.0f;

  if (isnan(radiation) || radiation < -100.0f ||
      radiation > (_cfg->nesaRsg1MaxWm2 * 1.20f)) {
    _data->nesaRsg1Ok = false;
    _data->nesaRsg1Failures++;
    _data->nesaRsg1LastError = "implausible_radiation";
    return;
  }

  if (radiation > _cfg->nesaRsg1MaxWm2) radiation = _cfg->nesaRsg1MaxWm2;

  _data->nesaRsg1Raw = raw;
  _data->nesaRsg1MilliVolts = volts * 1000.0f;
  _data->nesaRsg1RadiationWm2 = radiation;
  _data->nesaRsg1DetectedAddress = _cfg->nesaRsg1AdsAddress;
  _data->nesaRsg1Ok = true;
  _data->nesaRsg1LastError = "";
}

void SensorHub::sampleNesaSensors() {
  if (_cfg && _cfg->nesaTaEnabled && !_nesaTa) beginNesaTa();
  if (_cfg && _cfg->nesaRsg1Enabled && !_nesaRsg1) beginNesaRsg1();
  sampleNesaTa();
  sampleNesaRsg1();
}
