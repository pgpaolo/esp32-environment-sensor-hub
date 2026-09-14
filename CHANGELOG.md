# Changelog

## v0.7.3

Revisione robustezza, configurazione, diagnostica e manutenzione.

### Memoria / Web

- dashboard, configurazione e OTA spostate in `PROGMEM` tramite `src/WebAssets.h`;
- eliminate le grandi concatenazioni `String` per le pagine principali;
- JSON Web serializzato direttamente sul `WiFiClient`;
- diagnostica heap: free heap, min free heap, largest free block e frammentazione indicativa;
- fix compatibilità ArduinoJson: il `WiFiClient` viene ottenuto come lvalue prima della serializzazione.

### Configurazione

- schema NVS logico versione 1 (`cfgver`) nel namespace `sensorhub`;
- caricamento separato di `sensorhub_nesa` seguito da seconda validazione dell'oggetto completo;
- migrazione automatica delle configurazioni precedenti;
- validazione centralizzata di GPIO, indirizzi, intervalli, enum e parametri sensori;
- correzione selettiva dei soli valori non validi;
- password Web/Wi-Fi/MQTT e testo CA MQTT non restituiti da `/api/config`;
- campo password vuoto = mantiene il valore salvato;
- factory reset di `sensorhub` e `sensorhub_nesa`.

### MQTT

- contatori separati connect attempts / success / disconnects;
- contatori publish OK / failed;
- timestamp runtime per connect/disconnect/publish;
- reconnect con backoff progressivo fino a 60 s;
- LWT retained `online/offline` mantenuto;
- buffer payload JSON riutilizzato fra le pubblicazioni.

### Web UI / diagnostica

- pulsante PIN per ogni sensore;
- mappa pin completa;
- sensori abilitabili/disabilitabili singolarmente;
- diagnostica memoria e MQTT ampliata;
- AP manutenzione `192.168.4.1`;
- autenticazione factory `admin/admin`.

### OTA

- autenticazione HTTP Basic sul GET e sul POST;
- autenticazione anche nel callback che riceve i chunk prima di `Update.begin()/write()/end()`.

### NESA

- TA-N predisposto come **PT100 4 fili** tramite MAX31865;
- `R0 = 100 ohm`, `RREF = 430 ohm`, CS default GPIO13;
- documentata compatibilità con breakout MAX31865 PT100 Adafruit-compatible/DollaTek equivalenti;
- chiarito che MAX31855 non è compatibile con la RTD PT100;
- RSG1-N tramite ADS1115 differenziale A0-A1.

### Build verificata

```text
RAM   15,6% — 51.060 / 327.680 byte
Flash 57,8% — 1.135.421 / 1.966.080 byte
```

GitHub Actions `PlatformIO Build`: **SUCCESS** sull'HEAD verificato.

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
