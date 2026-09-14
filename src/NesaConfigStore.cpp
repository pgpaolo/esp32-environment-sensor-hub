#include "NesaConfigStore.h"
#include <Preferences.h>

void loadNesaConfig(AppConfig &c) {
  Preferences p;
  if (!p.begin("sensorhub_nesa", true)) return;
  c.nesaTaEnabled = p.getBool("ta_en", c.nesaTaEnabled);
  c.nesaTaCsPin = p.getUChar("ta_cs", c.nesaTaCsPin);
  c.nesaTaRtdNominalOhm = p.getFloat("ta_rtd", c.nesaTaRtdNominalOhm);
  c.nesaTaRefResistorOhm = p.getFloat("ta_ref", c.nesaTaRefResistorOhm);
  c.nesaTaTemperatureOffsetC = p.getFloat("ta_off", c.nesaTaTemperatureOffsetC);
  c.nesaRsg1Enabled = p.getBool("rsg_en", c.nesaRsg1Enabled);
  c.nesaRsg1AdsAddress = p.getUChar("rsg_a", c.nesaRsg1AdsAddress);
  c.nesaRsg1SensitivityUvPerWm2 = p.getFloat("rsg_s", c.nesaRsg1SensitivityUvPerWm2);
  c.nesaRsg1OffsetUv = p.getFloat("rsg_off", c.nesaRsg1OffsetUv);
  c.nesaRsg1MaxWm2 = p.getFloat("rsg_max", c.nesaRsg1MaxWm2);
  c.nesaRsg1ClampNegative = p.getBool("rsg_cl", c.nesaRsg1ClampNegative);
  p.end();
}

void saveNesaConfig(const AppConfig &c) {
  Preferences p;
  if (!p.begin("sensorhub_nesa", false)) return;
  p.putBool("ta_en", c.nesaTaEnabled);
  p.putUChar("ta_cs", c.nesaTaCsPin);
  p.putFloat("ta_rtd", c.nesaTaRtdNominalOhm);
  p.putFloat("ta_ref", c.nesaTaRefResistorOhm);
  p.putFloat("ta_off", c.nesaTaTemperatureOffsetC);
  p.putBool("rsg_en", c.nesaRsg1Enabled);
  p.putUChar("rsg_a", c.nesaRsg1AdsAddress);
  p.putFloat("rsg_s", c.nesaRsg1SensitivityUvPerWm2);
  p.putFloat("rsg_off", c.nesaRsg1OffsetUv);
  p.putFloat("rsg_max", c.nesaRsg1MaxWm2);
  p.putBool("rsg_cl", c.nesaRsg1ClampNegative);
  p.end();
}
