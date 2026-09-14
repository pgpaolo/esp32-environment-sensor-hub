#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>

#include "BuildInfo.h"
#include "AppConfig.h"
#include "RuntimeData.h"
#include "ConfigStore.h"
#include "SensorHub.h"
#include "MqttManager.h"
#include "WebUi.h"

RTC_DATA_ATTR uint32_t rtcBootCount = 0;

AppConfig cfg;
RuntimeData data;
ConfigStore store;
SensorHub sensors;
MqttManager mqtt;
WebUi *web = nullptr;

uint32_t lastSensorReadMs = 0;
uint32_t lastTelemetryMs = 0;
bool maintenanceMode = false;

static bool configButtonPressed(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
  delay(30);
  if (digitalRead(pin) != LOW) return false;
  const uint32_t start = millis();
  while (digitalRead(pin) == LOW && millis() - start < 1500) delay(10);
  return millis() - start >= 1200;
}

static String apSsid() {
  uint32_t id = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF);
  char b[32];
  snprintf(b, sizeof(b), "ESP32-Sensor-%06X", id);
  return String(b);
}

static void startNetwork() {
  maintenanceMode = configButtonPressed(cfg.configButtonPin) || cfg.wifiSsid.isEmpty();

  if (maintenanceMode) {
    WiFi.mode(WIFI_AP_STA);
    const String ssid = apSsid();
    WiFi.softAP(ssid.c_str());
    Serial.printf("[WiFi] AP manutenzione: %s, IP %s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_STA);
  }

  if (!cfg.wifiSsid.isEmpty()) {
    WiFi.setHostname(cfg.deviceName.c_str());
    WiFi.setAutoReconnect(true);

    if (cfg.wifiStaticIp) {
      IPAddress ip, gateway, subnet, dns1, dns2;
      const bool valid = ip.fromString(cfg.wifiIp) &&
                         gateway.fromString(cfg.wifiGateway) &&
                         subnet.fromString(cfg.wifiSubnet) &&
                         dns1.fromString(cfg.wifiDns1) &&
                         dns2.fromString(cfg.wifiDns2);
      if (valid) {
        if (!WiFi.config(ip, gateway, subnet, dns1, dns2)) {
          Serial.println("[WiFi] ERRORE applicazione configurazione IP statico");
        } else {
          Serial.printf("[WiFi] IP statico: %s GW=%s DNS=%s\n",
                        cfg.wifiIp.c_str(), cfg.wifiGateway.c_str(), cfg.wifiDns1.c_str());
        }
      } else {
        Serial.println("[WiFi] Configurazione IP statico non valida: uso DHCP");
      }
    }

    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());
    Serial.printf("[WiFi] Connessione a %s", cfg.wifiSsid.c_str());
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(300);
      Serial.print('.');
    }
    Serial.println();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] OK: %s RSSI=%d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    setenv("TZ", cfg.timezone.c_str(), 1);
    tzset();
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
    if (MDNS.begin(cfg.deviceName.c_str())) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[mDNS] http://%s.local/\n", cfg.deviceName.c_str());
    }
  } else if (!maintenanceMode) {
    WiFi.mode(WIFI_AP_STA);
    const String ssid = apSsid();
    WiFi.softAP(ssid.c_str());
    maintenanceMode = true;
    Serial.printf("[WiFi] STA fallita; AP fallback: %s, IP %s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);
  ++rtcBootCount;
  data.bootCount = rtcBootCount;

  store.load(cfg);
  Serial.printf("\n=== %s v%s | %s | boot #%lu ===\n", FW_NAME, FW_VERSION, cfg.deviceName.c_str(), (unsigned long)data.bootCount);

  startNetwork();
  sensors.begin(cfg, data);
  sensors.beginNesaSensors();
  mqtt.begin(cfg, data);

  web = new WebUi(cfg, data, store, mqtt, sensors);
  web->begin();

  const uint32_t now = millis();
  lastSensorReadMs = now;
  lastTelemetryMs = now - (cfg.telemetryIntervalSec * 1000UL) + 3000UL;
}

void loop() {
  if (web) web->loop();
  mqtt.loop();
  sensors.tick();

  data.wifiRssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  data.wifiIp = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();

  const uint32_t now = millis();
  const uint32_t sensorPeriodMs = max((uint32_t)2, cfg.sensorIntervalSec) * 1000UL;
  const uint32_t telemetryPeriodMs = max((uint32_t)5, cfg.telemetryIntervalSec) * 1000UL;

  if ((uint32_t)(now - lastSensorReadMs) >= sensorPeriodMs) {
    lastSensorReadMs = now;
    sensors.sampleSlowSensors();
    sensors.sampleNesaSensors();
  }

  if ((uint32_t)(now - lastTelemetryMs) >= telemetryPeriodMs) {
    lastTelemetryMs = now;
    mqtt.publishTelemetry("periodic");
  }

  if (sensors.takeImmediatePublishFlag()) {
    sensors.sampleSlowSensors();
    sensors.sampleNesaSensors();
    mqtt.publishTelemetry("sensor_event");
  }

  delay(2);
}
