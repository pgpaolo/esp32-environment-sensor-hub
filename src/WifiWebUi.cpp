#include "WifiWebUi.h"
#include "WebAssets.h"
#include "WifiSetupPage.h"

#include <WiFi.h>

void WifiWebUi::handleConfig() {
  if (!auth()) return;

  // The standard "Configurazione" entry always opens the Wi-Fi chooser first.
  // The complete legacy configuration remains one click away with ?advanced=1.
  if (!_server.hasArg("advanced")) {
    _server.send_P(200, "text/html; charset=utf-8", WifiSetupPage::PAGE);
    return;
  }

  _server.send_P(200, "text/html; charset=utf-8", WebAssets::CONFIG_PAGE);
}

void WifiWebUi::begin() {
  // Safe scan endpoint: active asynchronous scan with a much shorter dwell per
  // channel than the Arduino default. This reduces the time the single ESP32
  // radio is away from the SoftAP channel during first-boot provisioning.
  _server.on("/api/wifi/scan-safe", HTTP_POST, [this]() {
    if (!auth()) return;

    const int16_t scanState = WiFi.scanComplete();
    if (scanState == WIFI_SCAN_RUNNING) {
      _server.send(202, "application/json", "{\"status\":\"running\"}");
      return;
    }

    WiFi.scanDelete();
    constexpr uint32_t kMaxMsPerChannel = 120;
    const int16_t started = WiFi.scanNetworks(true, false, false, kMaxMsPerChannel);
    if (started == WIFI_SCAN_FAILED) {
      _server.send(500, "text/plain", "Impossibile avviare la scansione Wi-Fi");
      return;
    }

    _server.send(202, "application/json", "{\"status\":\"running\"}");
  });

  // Quick provisioning with safe update semantics for an already configured
  // device: blank password preserves the saved password when the SSID is the
  // same, and a static IPv4 configuration is preserved unless the SSID changes.
  _server.on("/api/wifi/configure-safe", HTTP_POST, [this]() {
    if (!auth()) return;
    if (!_server.hasArg("ssid")) {
      _server.send(400, "text/plain", "SSID mancante");
      return;
    }

    const String ssid = _server.arg("ssid");
    const String password = _server.hasArg("password") ? _server.arg("password") : String();
    if (ssid.isEmpty() || ssid.length() > 32) {
      _server.send(400, "text/plain", "SSID non valido");
      return;
    }
    if (password.length() > 64) {
      _server.send(400, "text/plain", "Password Wi-Fi troppo lunga");
      return;
    }

    const bool networkChanged = ssid != _cfg.wifiSsid;
    if (networkChanged || !password.isEmpty()) {
      _cfg.wifiPassword = password;
    }
    _cfg.wifiSsid = ssid;
    if (networkChanged) {
      _cfg.wifiStaticIp = false;
    }

    ConfigStore::validate(_cfg);
    _store.save(_cfg);

    _server.send(200, "text/plain",
                 networkChanged
                   ? "Nuova rete Wi-Fi salvata in DHCP; riavvio in corso"
                   : "Configurazione Wi-Fi aggiornata; riavvio in corso");
    delay(700);
    ESP.restart();
  });

  WebUi::begin();
}
