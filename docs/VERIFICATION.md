# Verifica generale develop / v0.7.3

Controllo funzionale, tecnico e documentale del repository `esp32-environment-sensor-hub` sul branch `develop`, dopo l'integrazione AdminSensor Remote e l'hardening OTA.

Baseline verificata:

```text
commit 49b2b7336ecea85783f915dcf8de38e5380045af
feat: add AdminSensor Remote and harden OTA
```

## Stato verificato

| Area | Verifica | Stato |
|---|---|---|
| Versione firmware | `FW_VERSION = 0.7.3` | OK |
| Build PlatformIO | `esp32dev` | OK |
| GitHub Actions | run 34855837984 | SUCCESS |
| RAM statica | 52.524 / 327.680 byte (16,0%) | OK |
| Flash applicativa | 1.210.381 / 1.966.080 byte (61,6%) | OK |
| Web statico | `src/WebAssets.h` in PROGMEM | OK |
| JSON Web | serializzazione diretta su `WiFiClient` | OK |
| Heap diagnostics | free/min/largest/fragmentation | OK |
| Config schema | `cfgver = 1` nel namespace `sensorhub` | OK |
| Merge NVS | `sensorhub` + `sensorhub_nesa` + seconda validazione | OK |
| Namespace remoto | `remote`: portal URL + token | OK |
| Token AdminSensor | random 256 bit, non esposto via API | OK |
| AdminSensor enrollment | HTTPS + pending/approved | OK |
| AdminSensor transport | WSS + Bearer token + heartbeat | OK |
| AdminSensor task | FreeRTOS `adminsensor`, stack 12.288 byte | OK |
| Proxy remoto | GET/POST/HEAD, path e limiti dimensionali | OK |
| Trust TLS remoto | ISRG Root X1/X2 | OK con vincolo CA |
| Password Web | non restituita da `/api/config` | OK |
| Password Wi-Fi | non restituita da `/api/config` | OK |
| Password MQTT | non restituita da `/api/config` | OK |
| CA MQTT | testo non restituito da `/api/config` | OK |
| MQTT LWT | retained `online/offline` | OK |
| MQTT backoff | progressivo fino a 60 s | OK |
| MQTT statistiche | connect/disconnect/publish separate | OK |
| Sensori disabilitabili | singolarmente da configurazione | OK |
| Pulsante PIN | card sensori + mappa completa | OK |
| AP manutenzione | `192.168.4.1` | OK |
| Web auth default | `admin/admin` | OK / da cambiare in produzione |
| OTA auth | GET, handler POST e callback upload autenticati | OK |
| NESA TA-N | MAX31865 / PT100 4 fili | OK |
| NESA RSG1-N | ADS1115 differenziale | OK |
| RF Oregon | assente dal progetto | OK |

## Build CI effettiva

L'ultima CI eseguita sul commit verificato ha prodotto:

```text
RAM:   16,0% — 52.524 / 327.680 byte
Flash: 61,6% — 1.210.381 / 1.966.080 byte
PlatformIO esp32dev: SUCCESS
```

I precedenti valori documentati di 51.060 byte RAM e 1.135.421 byte flash appartenevano alla baseline precedente all'integrazione completa di AdminSensor/WebSockets e sono stati riallineati nella documentazione corrente.

## NESA TA-N

Baseline verificata nel codice:

```text
MAX31865
MAX31865_4WIRE
R0   = 100 ohm
RREF = 430 ohm
CS   = GPIO13
```

Il firmware è predisposto per il TA-N/PT100 a 4 fili tramite MAX31865. Il MAX31865 è parte necessaria della catena di misura. MAX31855 non è compatibile con questa configurazione perché destinato alle termocoppie.

## NESA RSG1-N

Baseline:

```text
ADS1115
I2C 0x48
A0-A1 differenziale
GAIN_SIXTEEN
128 SPS
```

La sensibilità deve essere impostata secondo il certificato del piranometro.

## Configurazione

La configurazione principale è salvata in `sensorhub`; NESA in `sensorhub_nesa`; AdminSensor usa il namespace separato `remote`.

La chiave `cfgver` vive nel namespace principale ma governa lo schema logico della configurazione applicativa. Dopo il caricamento NESA viene eseguita una seconda validazione dell'oggetto completo.

La validazione copre GPIO, ADC1, bus I2C, indirizzi, intervalli generali, SDS011, AS3935 e parametri NESA.

## Web UI

Le pagine principali non richiedono grandi buffer `String` dinamici. Dashboard/configurazione/OTA sono servite da asset statici in flash.

`/api/status` include ora anche lo stato AdminSensor:

```text
configured
approved
transport_active
state
device_id
enroll_attempts
last_enroll_http_code
ws_connects
ws_disconnects
requests
responses
last_activity_age_ms
last_error
```

Le API dinamiche sono documentate in `docs/API.md`.

## MQTT

Il buffer PubSubClient è 4096 byte. Il payload corrente include i due NESA e le statistiche MQTT.

Il backoff si resetta al valore base dopo una connessione riuscita. Il payload applicativo riusa un buffer `String` riservato, riducendo allocazioni ripetute.

Nota di sicurezza: quando MQTT TLS è abilitato, il default firmware `mqttTlsInsecure=true` usa cifratura senza verifica del certificato finché l'opzione non viene modificata.

## AdminSensor Remote

### Identità

Il `device_id` è derivato dal MAC. Il token viene generato con `esp_fill_random()`, è lungo 32 byte e viene rappresentato come 64 caratteri esadecimali.

L'API espone solo `has_token=true/false`; il token non viene restituito.

### Enrollment e trasporto

Il firmware:

1. attende Wi-Fi;
2. attende un epoch valido per TLS;
3. invia enrollment HTTPS a `/api/device/enroll`;
4. gestisce `pending` e `approved`;
5. valida la URL WSS restituita;
6. apre il WebSocket con Bearer token;
7. usa heartbeat e reconnect.

### Proxy locale

Le richieste remote passano attraverso il WebServer locale e ricevono HTTP Basic con le credenziali Web correnti. Questo evita un secondo set di handler applicativi e mantiene il comportamento remoto coerente con l'amministrazione locale.

Limiti verificati:

```text
MAX_REQ  = 12.288 byte
MAX_RESP = 24.576 byte
MAX_WS   = 38.000 byte
```

## OTA

L'upload OTA è autenticato sia nell'handler finale sia nel callback che gestisce `UPLOAD_FILE_START`, `WRITE`, `END` e `ABORTED`. Un client non autenticato non deve quindi poter iniziare la scrittura della partizione.

## Osservazioni emerse dal controllo

### 1. Inizializzazione NTP dopo Wi-Fi ritardato — priorità alta

`configTime()` e mDNS vengono inizializzati nel bootstrap solo se lo STA risulta già connesso entro la finestra iniziale di circa 15 s.

Se la connessione Wi-Fi avviene successivamente tramite auto-reconnect, il firmware non richiama esplicitamente `configTime()`. Poiché AdminSensor rifiuta di procedere finché `time(nullptr)` non supera la soglia TLS, il sottosistema remoto può restare in `WAIT_TIME`.

**Raccomandazione:** rendere l'inizializzazione time/mDNS idempotente e richiamabile quando viene rilevata una transizione Wi-Fi a `WL_CONNECTED`.

### 2. Semantica factory reset — priorità media

`GET /factory` cancella `sensorhub` e `sensorhub_nesa`, ma non il namespace `remote`.

Inoltre un'operazione distruttiva è esposta come GET. Anche con Basic Auth è preferibile una POST con conferma/anti-CSRF applicativo.

**Raccomandazione:** decidere formalmente se il factory reset deve preservare l'identità AdminSensor o cancellare/disabilitare anche `remote`, quindi allineare endpoint e documentazione.

### 3. MQTT TLS insecure di default — priorità media

`mqttTlsInsecure=true` è il default. Se l'utente abilita soltanto MQTT TLS, la connessione è cifrata ma non autentica il broker.

**Raccomandazione:** in una futura revisione valutare default `false` e provisioning esplicito CA.

### 4. SoftAP senza WPA nella baseline — priorità media

Il SoftAP di manutenzione viene creato senza password Wi-Fi. La protezione applicativa è affidata a HTTP Basic.

**Raccomandazione:** usare credenziali Web robuste e valutare WPA2/WPA3 per l'AP di manutenzione.

### 5. Test automatici — priorità bassa

La CI verifica compilazione e dimensioni, ma non le macchine a stati o la validazione NVS.

**Raccomandazione:** introdurre unit test host-side per `ConfigStore::validate`, parsing URL e state transition pure dove separabili dall'hardware.

## Note di test hardware

La CI verifica compilazione e dimensioni, ma non sostituisce il collaudo sul dispositivo reale. Dopo il flash verificare:

1. avvio e reset reason;
2. heap/min heap/largest block dopo più accessi Web;
3. scansione I2C;
4. stato MQTT e LWT;
5. ciclo SDS011 sleep/wake;
6. fault handling MAX31865;
7. lettura ADS1115 con sensibilità reale;
8. pin map e sensori disabilitati;
9. salvataggio configurazione e riavvio;
10. factory reset e accesso `admin/admin`;
11. OTA autenticato e tentativo OTA non autenticato;
12. scollegamento/ricollegamento di sensori I2C e NESA senza reboot generale;
13. AdminSensor `PENDING -> APPROVED -> ONLINE`;
14. caduta e ripristino del WSS;
15. Wi-Fi inizialmente assente per oltre 15 s, successivo reconnect e verifica NTP/AdminSensor;
16. più handshake TLS e sessioni remote osservando `min_free_heap`;
17. soak test 24/72 h con sensori, MQTT, Web e AdminSensor contemporaneamente attivi.

## Esito

Il branch `develop` compila correttamente e l'integrazione AdminSensor risulta coerente con l'architettura esistente. La documentazione è stata riallineata al firmware reale e ai nuovi valori di build.

Non sono emersi errori di compilazione bloccanti. Restano da affrontare come hardening soprattutto il recupero NTP dopo connessione Wi-Fi ritardata e la semantica del factory reset rispetto al namespace AdminSensor.

Per la specifica estesa vedere [`ANALISI_FUNZIONALE_TECNICA.md`](ANALISI_FUNZIONALE_TECNICA.md).
