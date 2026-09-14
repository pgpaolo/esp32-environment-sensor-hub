# Architettura

ESP32 Environment Sensor Hub è un nodo ambientale locale basato su ESP32. Non contiene funzioni RF Oregon/Technoline: la parte radio è volutamente separata in un altro progetto.

## Flusso dati

```text
Sensori locali
   │
   ├─ I2C: BME280 / BH1750 / INA219 / AS3935 / ADS1115
   ├─ SPI: MAX31865 / NESA TA-N
   ├─ UART2: SDS011
   ├─ GPIO: DHT11 / AS3935 IRQ
   └─ ADC1: UV analogico
        │
        ▼
    SensorHub
        │
        ├─ validazione / plausibilità
        ├─ error counters / last_error
        ├─ stato runtime
        └─ eventi immediati
        │
        ├────────────► Web UI / API
        ├────────────► MQTT
        └────────────► Diagnostica
```

## Moduli principali

### `main.cpp`

Responsabilità:

- avvio seriale;
- caricamento configurazione principale;
- caricamento configurazione NESA;
- seconda validazione dell'oggetto configurazione completo;
- rete Wi-Fi/AP manutenzione;
- avvio sensori;
- avvio MQTT;
- avvio Web UI;
- scheduler base per sensori lenti e telemetria.

### `ConfigStore`

Gestisce il namespace NVS `sensorhub`:

- load/save;
- schema logico `cfgver`;
- validazione;
- correzione dei parametri non validi;
- factory reset del namespace principale.

### `NesaConfigStore`

Gestisce il namespace `sensorhub_nesa` e i parametri dei due sensori NESA. Non usa una chiave schema indipendente: i parametri NESA fanno parte dello stesso schema logico firmware e vengono validati dopo il merge con `sensorhub`.

### `SensorHub`

Gestisce i sensori standard e lo stato runtime. I sensori disabilitati non vengono inizializzati.

### `NesaSensors.cpp`

Contiene la logica dedicata a:

- NESA TA-N / PT100 tramite MAX31865, 4 fili;
- NESA RSG1-N tramite ADS1115.

### `MqttManager`

Gestisce:

- connessione broker;
- LWT retained;
- backoff reconnect;
- payload telemetria;
- buffer JSON riutilizzato;
- statistiche connect/disconnect/publish.

### `WebUi`

Gestisce WebServer, autenticazione HTTP Basic, API, configurazione, factory reset e OTA.

### `WebAssets.h`

Contiene dashboard/configurazione/OTA statiche in `PROGMEM` per ridurre l'uso e la frammentazione dell'heap.

## Rete

Modalità normale:

```text
ESP32 → Wi-Fi STA → LAN → Web/MQTT
```

Modalità manutenzione:

```text
ESP32 SoftAP
IP 192.168.4.1
Web Basic Auth
```

L'AP viene avviato se:

- manca una configurazione Wi-Fi valida;
- la connessione STA fallisce;
- il pulsante BOOT/config viene tenuto premuto all'avvio.

## Scheduler

Il firmware usa un loop principale leggero. Sensori e telemetria vengono eseguiti in base a `millis()`.

```text
loop
 ├─ WebServer.handleClient()
 ├─ MQTT loop/reconnect
 ├─ SensorHub.tick()
 ├─ campionamento sensori lenti a intervallo
 ├─ campionamento NESA a intervallo
 ├─ telemetria MQTT a intervallo
 └─ publish immediato su evento sensore
```

## SDS011

SDS011 usa una macchina a stati. L'ESP32 **non** entra in deep sleep.

```text
sleep
  ↓
wake
  ↓
warm-up
  ↓
sampling
  ↓
media
  ↓
publish/event
  ↓
sleep
```

## Fail-safe sensori

Un errore sensore non deve provocare il riavvio generale.

Ogni driver aggiorna:

- flag `ok`;
- contatore errori;
- ultimo errore;
- eventuale indirizzo rilevato;
- dati di misura solo quando validi.

Il sistema continua a servire Web UI e MQTT anche con uno o più sensori KO.

## Memoria

Le pagine statiche sono in flash/PROGMEM. Il JSON Web viene inviato direttamente al client.

Build di riferimento v0.7.3:

```text
RAM statica  51.060 / 327.680 byte = 15,6%
Flash        1.135.421 / 1.966.080 = 57,8%
```

La diagnostica runtime aggiunge heap libero, min heap, largest free block e frammentazione indicativa.

## Separazione dal progetto RF

Questo firmware non trasmette né riceve Oregon/Technoline e non implementa OSV3.

La separazione evita:

- timing RF critico nel nodo sensori;
- uso RAM/flash non necessario;
- accoppiamento fra acquisizione ambientale e gateway radio;
- regressioni RF dovute a modifiche Web/MQTT.
