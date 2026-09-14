# Changelog

## v0.7.3

Revisione robustezza e manutenzione.

### Memoria / Web

- dashboard e configurazione spostate in `PROGMEM` tramite `src/WebAssets.h`;
- eliminazione delle grandi concatenazioni `String` per le pagine principali;
- JSON Web serializzato direttamente sul client HTTP;
- diagnostica heap: free heap, min free heap, largest free block, frammentazione indicativa.

### Configurazione

- schema NVS versione 1 (`cfgver`);
- migrazione automatica delle configurazioni precedenti;
- validazione centralizzata di GPIO, indirizzi, intervalli e parametri sensori;
- correzione selettiva dei soli valori non validi;
- password Web/Wi-Fi/MQTT non restituite dalla configurazione;
- campo password vuoto = mantiene il valore salvato;
- factory reset dei namespace `sensorhub` e `sensorhub_nesa`.

### MQTT

- contatori separati connect attempts / success / disconnects;
- contatori publish OK / failed;
- timestamp runtime per connect/disconnect/publish;
- reconnect con backoff progressivo fino a 60 s;
- LWT retained `online/offline` mantenuto.

### Web UI / diagnostica

- pulsante PIN per ogni sensore;
- mappa pin completa;
- sensori abilitabili/disabilitabili singolarmente;
- diagnostica memoria e MQTT ampliata;
- AP manutenzione `192.168.4.1`;
- autenticazione factory `admin/admin`.

### Fix

- corretto streaming JSON verso `WiFiClient` per compatibilità con ArduinoJson/Arduino ESP32;
- build PlatformIO `esp32dev` verificata con successo.

Build di riferimento:

```text
RAM   15,6% — 51.036 / 327.680 byte
Flash 57,7% — 1.134.213 / 1.966.080 byte
```

## v0.7.2

- autenticazione Web `admin/admin`;
- AP manutenzione `192.168.4.1`;
- abilitazione/disabilitazione sensori;
- pulsante PIN e documentazione pinout;
- supporto iniziale NESA TA-N e NESA RSG1-N.

## v0.7.x — integrazione NESA

- NESA TA-N tramite MAX31865 / PT100 4 fili;
- NESA RSG1-N tramite ADS1115 differenziale;
- diagnostica e MQTT dedicati ai due sensori.

## v0.6.x — baseline Sensor Hub

- BME280, DHT11, BH1750, INA219, UV, SDS011, AS3935;
- Web UI stile Oregon/Technoline;
- configurazione NVS;
- OTA;
- MQTT;
- diagnostica sensori;
- SDS011 con sleep/wake non bloccante e ESP32 sempre acceso.
