# Analisi funzionale e tecnica

Documento di riferimento per il branch `develop` di **ESP32 Environment Sensor Hub**.

Baseline analizzata: commit `49b2b7336ecea85783f915dcf8de38e5380045af` (`feat: add AdminSensor Remote and harden OTA`), firmware `0.7.3`.

## 1. Obiettivo del sistema

ESP32 Environment Sensor Hub è un nodo embedded per acquisizione ambientale locale, diagnostica e telemetria. Il firmware deve continuare a fornire le funzioni di rete e amministrazione anche quando uno o più sensori risultano assenti o guasti.

Il progetto copre:

- acquisizione di sensori locali I2C, SPI, UART, GPIO e ADC1;
- normalizzazione e validazione delle letture;
- pubblicazione MQTT JSON;
- Web UI e API HTTP protette da autenticazione;
- configurazione persistente in NVS;
- gestione relay;
- diagnostica runtime di sensori, rete, heap e MQTT;
- OTA da browser;
- accesso remoto opzionale tramite **AdminSensor Remote**;
- modalità SoftAP di manutenzione.

La ricezione RF Oregon Scientific/Technoline è volutamente esclusa e resta responsabilità del gateway RF dedicato.

## 2. Attori e interfacce

| Attore / sistema | Interfaccia | Funzione |
|---|---|---|
| Operatore locale | HTTP TCP/80 | Dashboard, configurazione, diagnostica, relay, SDS011, OTA |
| Broker MQTT | TCP/1883 o TLS | Telemetria, availability/LWT |
| AdminSensor Portal | HTTPS + WSS | Enrollment, approvazione dispositivo e tunnel amministrativo remoto |
| Sensori I2C | I2C | BME280, BH1750, INA219, AS3935, ADS1115 |
| NESA TA-N | SPI/MAX31865 | Temperatura PT100 4 fili |
| SDS011 | UART2 | PM2.5/PM10 con ciclo sleep/wake |
| DHT11 | GPIO | Temperatura/umidità |
| UV analogico | ADC1 | Tensione e UV Index |
| Relay | GPIO output | Attuatore locale |
| NTP | UDP/IP | Sincronizzazione temporale necessaria anche per TLS |

## 3. Requisiti funzionali

### RF-01 — Avvio e caricamento configurazione

All'avvio il firmware deve:

1. inizializzare seriale e boot counter RTC;
2. caricare `sensorhub`;
3. caricare `sensorhub_nesa`;
4. validare l'oggetto `AppConfig` completo;
5. correggere e persistere valori non validi;
6. inizializzare rete, sensori, MQTT, Web UI e AdminSensor Remote.

La validazione deve evitare configurazioni elettricamente o logicamente impossibili, inclusi GPIO non utilizzabili, indirizzi fuori range e conflitti I2C INA219/ADS1115.

### RF-02 — Acquisizione sensori

Un sensore disabilitato non deve essere inizializzato, interrogato o considerato guasto.

Per ogni sensore attivo il runtime mantiene almeno:

- flag `ok`;
- valore/i correnti;
- contatore errori quando previsto;
- ultimo errore;
- indirizzo rilevato per i dispositivi I2C quando applicabile.

Il guasto di un singolo sensore non deve fermare Web UI, MQTT o gli altri sensori.

### RF-03 — Recupero sensori

I driver predisposti al recupero devono tentare la reinizializzazione senza reboot generale quando il sensore torna disponibile. Questo comportamento è particolarmente rilevante per BH1750, BME280, INA219, AS3935 e sensori NESA.

### RF-04 — SDS011

SDS011 utilizza una macchina a stati non bloccante:

```text
sleeping -> waking -> warming -> sampling -> sleeping
```

Il dispositivo ESP32 resta sempre acceso. Sono previsti:

- warm-up configurabile;
- numero campioni configurabile;
- media dei campioni validi;
- timeout massimo awake;
- misura manuale fuori ciclo;
- sleep forzato;
- publish immediato a fine ciclo.

### RF-05 — AS3935

L'AS3935 usa I2C e interrupt GPIO. Gli eventi devono essere processati nel loop, aggiornare diagnostica e richiedere una telemetria immediata senza eseguire elaborazioni pesanti nell'ISR.

### RF-06 — MQTT

Il firmware pubblica:

```text
<base_topic>/status
<base_topic>/telemetry
```

`status` implementa availability retained e LWT `offline`. La riconnessione usa backoff crescente fino a 60 s. Il payload telemetrico include dati sensori, stato relay e diagnostica di sistema/MQTT.

### RF-07 — Web UI e API

Tutti gli endpoint applicativi richiedono HTTP Basic Authentication. Le API devono permettere:

- lettura stato runtime;
- lettura configurazione non sensibile;
- scansione I2C;
- controllo relay;
- controllo SDS011;
- configurazione;
- factory reset;
- OTA;
- configurazione e diagnostica AdminSensor Remote.

Password Wi-Fi, MQTT e Web e il testo del certificato CA MQTT non devono essere restituiti da `/api/config`.

### RF-08 — OTA

L'OTA HTTP deve autenticare sia l'handler POST finale sia il callback che riceve i chunk **prima** di `Update.begin()`, `Update.write()` e `Update.end()`.

### RF-09 — Modalità manutenzione

Il SoftAP viene attivato quando:

- non esiste una configurazione Wi-Fi valida;
- la connessione STA iniziale fallisce;
- il pulsante BOOT/config viene mantenuto premuto all'avvio.

IP previsto: `192.168.4.1`.

### RF-10 — AdminSensor Remote

AdminSensor Remote è opzionale e disabilitato quando `portalUrl` è vuoto.

Il firmware deve:

1. generare un `device_id` stabile derivato dal MAC;
2. generare un token casuale a 256 bit e salvarlo nel namespace NVS `remote`;
3. accettare dall'installatore soltanto la base URL HTTPS del portale;
4. attendere Wi-Fi e tempo valido prima di usare TLS;
5. eseguire enrollment HTTPS su `/api/device/enroll`;
6. gestire gli stati `pending`, `approved`, `denied/error`;
7. aprire il WebSocket WSS restituito dal portale solo dopo approvazione;
8. usare `Authorization: Bearer <device_token>` sul WSS;
9. inoltrare richieste amministrative verso la Web UI locale usando le credenziali Web correnti;
10. limitare metodi, path e dimensioni delle richieste/risposte;
11. non esporre il token tramite API Web.

## 4. Architettura logica

```text
                         +----------------------+
                         |   AdminSensor Portal |
                         |  HTTPS enrollment    |
                         |  WSS admin tunnel    |
                         +----------+-----------+
                                    |
                                    v
+-----------+      +----------------+----------------+
| Sensori   |----->|            ESP32                |
| locali    |      |                                  |
+-----------+      |  SensorHub ----> RuntimeData     |
                   |      |                |           |
                   |      |                +--> Web UI/API
                   |      |                +--> MQTT
                   |      |                +--> Diagnostics
                   |      |                            |
                   |      +--> event publish ----------+
                   |                                  |
                   |  ConfigStore / NesaConfigStore   |
                   |         |                        |
                   |         +--> NVS                 |
                   |                                  |
                   |  AdminSensor task --local HTTP-->WebServer
                   +----------------------------------+
```

## 5. Moduli software

### `src/main.cpp`

Orchestrazione del boot e scheduler principale. Mantiene gli intervalli per sensori lenti e telemetria e richiama in ogni iterazione WebServer, MQTT e `SensorHub::tick()`.

### `include/AppConfig.h`

Contratto della configurazione persistente e dei default firmware.

### `include/RuntimeData.h`

Stato volatile corrente. Separa le misure e la diagnostica dalla configurazione.

### `ConfigStore`

Persistenza del namespace `sensorhub`, schema `cfgver`, migrazione implicita tramite default+validate e validazione centralizzata.

### `NesaConfigStore`

Persistenza NESA nel namespace `sensorhub_nesa`. I parametri vengono validati nel contesto dell'`AppConfig` completo.

### `SensorHub`

Driver e coordinamento dei sensori standard, SDS011, AS3935 e relay.

### `NesaSensors.cpp`

Acquisizione NESA TA-N/MAX31865 e RSG1-N/ADS1115.

### `MqttManager`

Connessione broker, TLS opzionale, LWT, backoff, composizione payload JSON e contatori runtime.

### `WebUi`

Server HTTP, autenticazione, API, configurazione, diagnostica, factory reset e OTA.

### `WebAssets.h`

HTML/CSS/JavaScript statici in PROGMEM.

### `remote_access.cpp`

Sottosistema AdminSensor. Esegue in task FreeRTOS dedicato `adminsensor` con stack richiesto di 12.288 byte, mantiene enrollment/WSS e inoltra le richieste verso HTTP locale.

### `remote_trust.h`

Trust store compilato nel firmware per AdminSensor Remote. La baseline contiene ISRG Root X1 e X2; quindi il portale deve presentare una catena TLS compatibile con tali trust anchor.

## 6. Modello di concorrenza

Il firmware usa due contesti principali:

```text
Arduino loop task
  - WebServer.handleClient
  - MQTT loop/reconnect
  - SensorHub tick
  - scheduler sensori
  - publish telemetria

FreeRTOS task "adminsensor"
  - HTTPS enrollment
  - WebSocket WSS
  - proxy HTTP verso WebServer locale
```

Lo stato AdminSensor condiviso viene protetto da mutex FreeRTOS. Il tunnel remoto non accede direttamente ai driver sensore: usa l'interfaccia HTTP locale, mantenendo un unico punto di autorizzazione e di comportamento applicativo.

## 7. Persistenza

| Namespace | Contenuto |
|---|---|
| `sensorhub` | configurazione generale, rete, Web, MQTT, GPIO, sensori standard |
| `sensorhub_nesa` | parametri NESA TA-N/RSG1-N |
| `remote` | URL portale AdminSensor e token dispositivo |

Il token AdminSensor non è configurabile dall'utente e non viene restituito dalle API.

## 8. Flusso dati

### Campionamento periodico

```text
millis()
  -> sensorIntervalSec
  -> sampleSlowSensors()
  -> sampleNesaSensors()
  -> RuntimeData
```

### Telemetria

```text
RuntimeData + AppConfig
  -> ArduinoJson
  -> buffer String riutilizzato
  -> PubSubClient
  -> <base_topic>/telemetry
```

### Evento immediato

Un evento sensore o cambio relay imposta `_immediatePublish`; il loop acquisisce i sensori lenti/NESA e pubblica con reason `sensor_event`.

## 9. API HTTP principali

| Metodo | Endpoint | Funzione |
|---|---|---|
| GET | `/` | Dashboard |
| GET | `/config` | Configurazione |
| GET | `/update` | Form OTA |
| POST | `/update` | Upload firmware |
| GET | `/api/status` | Stato completo |
| GET | `/api/config` | Configurazione non sensibile |
| GET | `/api/i2c` | Scan I2C |
| POST | `/api/relay/toggle` | Toggle relay |
| POST | `/api/sds/measure` | Misura SDS011 manuale |
| POST | `/api/sds/sleep` | Sleep SDS011 |
| GET | `/api/remote/config` | Configurazione AdminSensor non sensibile |
| POST | `/api/remote/config` | Salva/disabilita URL portale |
| GET | `/api/remote/status` | Stato enrollment/WSS |
| POST | `/api/remote/retry` | Retry immediato |
| POST | `/api/remote/reset` | Disabilita portale remoto mantenendo identità/token |
| POST | `/save` | Salva configurazione e riavvia |
| GET | `/factory` | Reset configurazione principale/NESA |

## 10. Sicurezza

### Controlli presenti

- HTTP Basic su Web UI e API;
- password e CA MQTT non restituiti dalla API di configurazione;
- OTA autenticato anche durante la ricezione dei chunk;
- AdminSensor solo via HTTPS/WSS;
- token AdminSensor casuale 256 bit;
- token non esposto dalla Web UI;
- approvazione portale prima dell'apertura del tunnel;
- trust anchor TLS compilati nel firmware;
- limiti di dimensione per proxy remoto;
- path remoto filtrato e metodi limitati a GET/POST/HEAD.

### Confini di fiducia da considerare

1. **Web locale non cifrato.** HTTP Basic su TCP/80 protegge l'accesso logico ma non la confidenzialità sulla LAN.
2. **Credenziali NVS non cifrate.** Wi-Fi, MQTT e Web sono salvate in NVS in chiaro a livello applicativo.
3. **MQTT TLS insecure.** Il default `mqttTlsInsecure=true` disabilita la verifica del certificato quando TLS viene abilitato senza cambiare tale opzione.
4. **SoftAP senza WPA nella baseline.** La protezione applicativa resta HTTP Basic; le credenziali factory `admin/admin` devono essere cambiate in installazioni reali.
5. **AdminSensor equivale a un amministratore remoto approvato.** Una volta ONLINE, il portale può invocare gli endpoint locali con le credenziali Web del dispositivo. La sicurezza del portale e del processo di approval diventa parte del perimetro del dispositivo.
6. **Trust store AdminSensor limitato.** La configurabilità dell'URL non equivale a compatibilità con qualsiasi CA pubblica: la baseline si fida di ISRG Root X1/X2.

## 11. Robustezza e failure mode

| Evento | Comportamento previsto |
|---|---|
| Sensore I2C assente | errore locale, contatore/last_error, firmware operativo |
| Sensore che ritorna online | reinizializzazione dove implementata |
| MQTT indisponibile | Web/sensori operativi, retry con backoff |
| Wi-Fi assente al boot | SoftAP manutenzione |
| SDS011 non risponde | timeout, sleep, ciclo successivo |
| AS3935 assente | retry periodico senza reboot generale |
| AdminSensor portale non raggiungibile | stato ERROR/retry, nodo locale operativo |
| AdminSensor pending | retry enrollment ogni 30 s |
| WSS cade | stato RECONNECT e reconnect automatico |
| Tempo non valido | AdminSensor resta `WAIT_TIME` e non apre TLS |
| OTA non autenticato | nessuna scrittura firmware |

## 12. Budget memoria e build

Ultima CI verificata sulla baseline `49b2b733...`:

```text
PlatformIO esp32dev: SUCCESS
RAM statica : 52.524 / 327.680 byte = 16,0%
Flash       : 1.210.381 / 1.966.080 byte = 61,6%
```

Rispetto alla baseline precedente, AdminSensor/WebSockets aumenta soprattutto l'uso flash. Il margine statico resta ampio, ma il dato RAM della build non include integralmente le allocazioni dinamiche runtime, tra cui:

- stack task `adminsensor` da 12.288 byte;
- buffer WebSocket/HTTP;
- `String` e `std::vector` usati dal tunnel;
- heap TLS durante handshake HTTPS/WSS.

Per il collaudo reale sono quindi più significativi anche `free_heap`, `min_free_heap` e `largest_free_block` esposti da `/api/status`.

## 13. Esito del controllo

### Corretto/documentato in questa revisione

- integrazione AdminSensor inserita nell'architettura ufficiale;
- API remote documentate;
- modello concorrente esplicitato;
- namespace `remote` aggiunto al modello di persistenza;
- trust boundary e limiti TLS descritti;
- valori RAM/flash riallineati alla CI reale del branch `develop`;
- requisiti funzionali e tecnici raccolti in un unico documento verificabile.

### Osservazioni tecniche da valutare nei prossimi hardening

| Priorità | Osservazione | Impatto |
|---|---|---|
| Alta | NTP/mDNS vengono inizializzati solo se STA è già connessa al termine del bootstrap di rete | se il Wi-Fi si collega solo successivamente, AdminSensor può restare `WAIT_TIME`; da rendere idempotente nel loop/reconnect |
| Media | `/factory` è un endpoint GET distruttivo e non cancella/disabilita esplicitamente il namespace `remote` | semantica factory reset e superficie CSRF da rivedere |
| Media | `mqttTlsInsecure` è `true` di default | TLS può cifrare senza autenticare il broker se non riconfigurato |
| Media | SoftAP non usa password WPA nella baseline | richiede password Web forte, soprattutto fuori da ambienti controllati |
| Bassa | CI verifica compilazione ma non esistono test automatici di stato macchina/config validation | regressioni logiche richiedono collaudo hardware/manuale |

Le prime due osservazioni sono buoni candidati per la prossima revisione firmware perché riguardano comportamento di recupero rete e semantica di reset, non il normale percorso di acquisizione sensori.

## 14. Criteri di collaudo consigliati

1. boot con Wi-Fi disponibile e non disponibile;
2. connessione Wi-Fi ritardata oltre i 15 s iniziali;
3. sincronizzazione NTP e stato AdminSensor dopo reconnect;
4. enrollment AdminSensor `pending -> approved -> ONLINE`;
5. caduta e ripristino WSS;
6. accesso remoto a dashboard/API senza esposizione del token;
7. test heap prima/dopo più handshake TLS e sessioni remote;
8. MQTT plain, TLS insecure e TLS con CA;
9. disconnessione/riconnessione di ogni sensore recuperabile;
10. ciclo SDS011 normale, timeout e misura manuale;
11. evento AS3935 con publish immediato;
12. salvataggio NVS, reboot e migrazione/validazione;
13. OTA autenticato e tentativo OTA senza credenziali;
14. factory reset e verifica puntuale dei namespace residui;
15. soak test 24/72 h con MQTT, Web UI e AdminSensor attivi.
