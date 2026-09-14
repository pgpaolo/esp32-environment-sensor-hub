# Architettura

ESP32 Environment Sensor Hub è un nodo ambientale locale basato su ESP32. Non contiene funzioni RF Oregon/Technoline: la parte radio è volutamente separata in un altro progetto.

La baseline `develop` analizzata include anche **AdminSensor Remote**, che aggiunge un canale amministrativo remoto HTTPS/WSS senza modificare il flusso di acquisizione dei sensori.

## Vista generale

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
        ├────────────► Web UI / API ◄────────────┐
        ├────────────► MQTT                      │
        └────────────► Diagnostica               │
                                                  │ HTTP locale autenticato
AdminSensor Portal                                │
   │ HTTPS enrollment                             │
   └─ WSS autenticato ─► remote_access task ─────┘
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
- avvio AdminSensor Remote;
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

Gestisce i sensori standard, relay e stato runtime. I sensori disabilitati non vengono inizializzati.

### `NesaSensors.cpp`

Contiene la logica dedicata a:

- NESA TA-N / PT100 tramite MAX31865, 4 fili;
- NESA RSG1-N tramite ADS1115.

### `MqttManager`

Gestisce:

- connessione broker;
- TLS opzionale;
- LWT retained;
- backoff reconnect;
- payload telemetria;
- buffer JSON riutilizzato;
- statistiche connect/disconnect/publish.

### `WebUi`

Gestisce WebServer, autenticazione HTTP Basic, API, configurazione, diagnostica, factory reset e OTA.

### `WebAssets.h`

Contiene dashboard/configurazione/OTA statiche in `PROGMEM` per ridurre l'uso e la frammentazione dell'heap.

### `remote_access.cpp`

Gestisce AdminSensor Remote in un task FreeRTOS dedicato.

Responsabilità:

- generazione dell'identità dispositivo;
- gestione token per-device in NVS;
- validazione della base URL HTTPS;
- enrollment verso il portale;
- gestione approval/pending;
- connessione WSS autenticata con Bearer token;
- heartbeat e reconnect;
- proxy delle richieste remote verso il WebServer locale;
- statistiche e stato diagnostico.

Il task `adminsensor` viene creato con stack richiesto di 12.288 byte e priorità 1.

### `remote_trust.h`

Contiene i trust anchor usati da AdminSensor Remote per HTTPS/WSS. La baseline include ISRG Root X1 e ISRG Root X2.

La URL del portale è configurabile a runtime, ma il certificato deve comunque presentare una catena compatibile con i root inclusi nel firmware.

## Rete

### Modalità normale

```text
ESP32 → Wi-Fi STA → LAN → Web/MQTT
                    │
                    └→ Internet → AdminSensor HTTPS/WSS
```

### Modalità manutenzione

```text
ESP32 SoftAP
IP 192.168.4.1
Web Basic Auth
```

L'AP viene avviato se:

- manca una configurazione Wi-Fi valida;
- la connessione STA iniziale fallisce;
- il pulsante BOOT/config viene tenuto premuto all'avvio.

## Scheduler e concorrenza

Il firmware usa un loop Arduino leggero e un task FreeRTOS separato per AdminSensor.

### Arduino loop

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

### Task AdminSensor

```text
adminsensor task
 ├─ attesa Wi-Fi
 ├─ attesa tempo valido
 ├─ enrollment HTTPS
 ├─ gestione pending/approved
 ├─ WebSocket WSS
 ├─ heartbeat/reconnect
 └─ proxy HTTP locale
```

Lo stato condiviso del sottosistema remoto è protetto da mutex FreeRTOS. Il task remoto non accede direttamente ai driver sensore: utilizza la Web API locale, mantenendo il WebServer come unico punto applicativo di amministrazione.

## Macchina a stati AdminSensor

Flusso nominale:

```text
OFF
 ↓ portal URL configurata
WAIT_NETWORK
 ↓ Wi-Fi disponibile
WAIT_TIME
 ↓ epoch valido
ENROLLING
 ├─ pending  → PENDING → retry enrollment
 ├─ denied   → DENIED
 └─ approved → APPROVED → CONNECTING → ONLINE
                                      │
                                      └─ perdita WSS → RECONNECT
```

Intervalli correnti:

```text
pending retry   : 30 s
error retry     : 15 s
approved check  : 60 s
WSS reconnect   : 5 s
heartbeat       : 30 s, timeout 5 s, 2 tentativi
```

## Proxy remoto

Il portale non dialoga direttamente con i driver. Ogni richiesta approvata viene inoltrata al WebServer locale su TCP/80 con HTTP Basic generato dalle credenziali Web correnti del dispositivo.

Controlli principali:

- metodi ammessi: `GET`, `POST`, `HEAD`;
- path relativo obbligatorio;
- blocco di CR/LF e URL assolute nel path;
- richiesta massima: 12.288 byte;
- risposta locale massima: 24.576 byte;
- messaggio WebSocket massimo: 38.000 byte.

## SDS011

SDS011 usa una macchina a stati. L'ESP32 **non** entra in deep sleep.

```text
sleeping
  ↓
waking
  ↓
warming
  ↓
sampling
  ↓
media
  ↓
publish/event
  ↓
sleeping
```

Il ciclo ha timeout massimo awake e può essere avviato o interrotto manualmente tramite API.

## Fail-safe sensori

Un errore sensore non deve provocare il riavvio generale.

Ogni driver aggiorna, dove applicabile:

- flag `ok`;
- contatore errori;
- ultimo errore;
- eventuale indirizzo rilevato;
- dati di misura solo quando validi.

Il sistema continua a servire Web UI e MQTT anche con uno o più sensori KO. Diversi driver tentano inoltre la reinizializzazione quando il dispositivo torna disponibile.

## Persistenza

```text
sensorhub       → configurazione generale
sensorhub_nesa  → configurazione NESA
remote          → AdminSensor portal URL + token per-device
```

La chiave `cfgver` governa lo schema logico della configurazione applicativa principale. AdminSensor mantiene il proprio namespace separato.

## Memoria

Le pagine statiche sono in flash/PROGMEM. Il JSON Web viene inviato direttamente al client e il payload MQTT riusa un buffer applicativo.

Build verificata sul commit `49b2b7336ecea85783f915dcf8de38e5380045af`:

```text
RAM statica  52.524 / 327.680 byte = 16,0%
Flash        1.210.381 / 1.966.080 = 61,6%
```

Questi numeri non includono tutte le allocazioni runtime. AdminSensor aggiunge, tra l'altro, stack task, heap TLS, buffer WebSocket e `std::vector` temporanei. Per il soak test vanno quindi osservati anche free heap, minimum free heap e largest free block.

## Considerazioni di sicurezza

- la Web UI locale usa HTTP Basic su HTTP non cifrato;
- le credenziali applicative sono memorizzate in NVS senza cifratura applicativa;
- MQTT può essere configurato in TLS, ma `mqttTlsInsecure=true` è il default firmware;
- AdminSensor usa invece TLS con verifica basata sui root compilati;
- una sessione AdminSensor approvata ha capacità amministrative equivalenti agli endpoint Web locali esposti dal proxy;
- la sicurezza del portale e del workflow di approvazione fa quindi parte del trust boundary del dispositivo.

## Separazione dal progetto RF

Questo firmware non trasmette né riceve Oregon/Technoline e non implementa OSV3.

La separazione evita:

- timing RF critico nel nodo sensori;
- uso RAM/flash non necessario;
- accoppiamento fra acquisizione ambientale e gateway radio;
- regressioni RF dovute a modifiche Web/MQTT/AdminSensor.

Per la specifica completa vedere [`ANALISI_FUNZIONALE_TECNICA.md`](ANALISI_FUNZIONALE_TECNICA.md).
