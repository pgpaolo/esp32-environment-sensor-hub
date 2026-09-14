# Web UI e API HTTP

ESP32 Environment Sensor Hub **v0.7.3** espone Web UI e API sulla porta TCP 80.

Tutti gli endpoint applicativi, inclusi i callback di upload OTA e le API AdminSensor Remote, sono protetti dalla stessa autenticazione HTTP Basic.

Credenziali factory:

```text
admin / admin
```

## Pagine Web

| Metodo | Endpoint | Descrizione |
|---|---|---|
| GET | `/` | Dashboard sensori e diagnostica |
| GET | `/config` | Pagina configurazione |
| GET | `/update` | Pagina OTA |
| POST | `/update` | Upload `firmware.bin` autenticato |

Le pagine statiche principali sono memorizzate in PROGMEM in `src/WebAssets.h` e servite con `send_P()`.

## GET `/api/status`

Restituisce lo stato runtime. Sezioni principali:

- `system`: Wi-Fi, IP, RSSI, heap, flash, firmware, reset reason, config schema;
- `remote`: stato AdminSensor Remote;
- `mqtt`: stato, tentativi, successi, disconnessioni, publish, timestamp e backoff;
- `pins`: GPIO configurati/effettivi;
- `relay` e tutti i blocchi sensore.

Esempio parziale:

```json
{
  "system": {
    "wifi": true,
    "ip": "192.168.1.221",
    "rssi_dbm": -63,
    "mqtt": true,
    "free_heap": 180000,
    "min_free_heap": 130000,
    "largest_free_block": 110000,
    "heap_fragmentation_pct": 38.8,
    "firmware": "0.7.3",
    "config_schema": 1
  },
  "remote": {
    "configured": true,
    "approved": true,
    "transport_active": true,
    "state": "ONLINE",
    "device_id": "esp32-aabbccddeeff"
  },
  "mqtt": {
    "connected": true,
    "connect_attempts": 3,
    "connect_success": 2,
    "disconnects": 1,
    "publish_ok": 120,
    "publish_failed": 1,
    "backoff_s": 5
  }
}
```

Il JSON viene serializzato direttamente sul `WiFiClient`, evitando una seconda grande copia in RAM.

## GET `/api/config`

Restituisce i parametri necessari alla pagina configurazione ma **non** restituisce:

- password Wi-Fi;
- password MQTT;
- password Web;
- testo CA MQTT.

Espone soltanto i flag:

```text
wifi_password_set
mqtt_password_set
web_password_set
mqtt_ca_set
```

## GET `/api/i2c`

Restituisce lo scan testuale I2C e identifica i dispositivi noti. ADS1115/RSG1-N viene riconosciuto; AS3935 `0x00` viene segnalato come general-call ma non interrogato attivamente dallo scanner.

Il MAX31865 del NESA TA-N non compare nello scan I2C perché usa SPI.

## POST `/api/relay/toggle`

Commuta il relay configurato. Se il relay è disabilitato, la richiesta non modifica l'uscita.

## POST `/api/sds/measure`

Richiede una misura SDS011 fuori ciclo. Risponde `200` se accettata, `409` se lo stato corrente non lo permette.

## POST `/api/sds/sleep`

Forza SDS011 allo stato sleep. Risponde `200` in caso di successo, `409` se non applicabile.

## AdminSensor Remote

AdminSensor Remote è configurabile e diagnosticabile tramite API locali. Il **device token non viene mai restituito**.

### GET `/api/remote/config`

Restituisce la configurazione non sensibile, ad esempio:

```json
{
  "portal_url": "https://admin.example.net",
  "device_id": "esp32-aabbccddeeff",
  "has_token": true,
  "identity_managed_by_firmware": true
}
```

Campi:

- `portal_url`: base URL del portale;
- `device_id`: identità stabile derivata dal MAC;
- `has_token`: indica soltanto se è presente un token valido;
- `identity_managed_by_firmware`: conferma che identità/token non sono editabili dall'installatore.

### POST `/api/remote/config`

Parametro form:

```text
url=https://admin.example.net
```

Vincoli:

- solo schema HTTPS;
- nessuna query string;
- nessun fragment;
- lunghezza massima validata;
- URL vuota = AdminSensor Remote disabilitato.

Il salvataggio non rigenera il token per-device.

### GET `/api/remote/status`

Restituisce diagnostica del sottosistema remoto:

```text
initialized
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

Stati tipici:

```text
OFF
WAIT_NETWORK
WAIT_TIME
ENROLLING
PENDING
APPROVED
CONNECTING
ONLINE
RECONNECT
DENIED
ERROR
```

### POST `/api/remote/retry`

Forza un nuovo tentativo di enrollment/reconnect senza modificare identità o token.

### POST `/api/remote/reset`

Disabilita AdminSensor Remote cancellando la URL del portale. Il token per-device viene deliberatamente preservato, così la stessa unità mantiene l'identità se riconfigurata successivamente.

### Tunnel remoto

Dopo l'approvazione lato portale il firmware apre una sessione WSS autenticata con `Authorization: Bearer <device_token>`.

Le richieste ricevute dal tunnel sono inoltrate al WebServer locale con le credenziali Web correnti del dispositivo. Sono ammessi soltanto:

```text
GET
POST
HEAD
```

Sono inoltre applicati controlli sul path e limiti dimensionali:

```text
request body massimo   12.288 byte
risposta locale massima 24.576 byte
messaggio WS massimo    38.000 byte
```

### TLS AdminSensor

HTTPS e WSS usano i trust anchor compilati in `src/remote_trust.h`. La baseline contiene ISRG Root X1/X2. Una URL sintatticamente valida può quindi non essere raggiungibile se la catena TLS del portale non è compatibile con questi root.

## POST `/save`

Riceve il form di configurazione, applica la validazione, salva i namespace NVS interessati e riavvia.

```text
password vuota  → mantiene il valore corrente
nuovo valore    → sostituisce
clear flag      → cancella, ove previsto
```

Il reset credenziali Web riporta `admin/admin`.

La validazione finale viene eseguita sull'oggetto configurazione completo, compresi i parametri NESA.

## GET `/factory`

Cancella nella baseline corrente:

```text
sensorhub
sensorhub_nesa
```

quindi riavvia con i default firmware. L'endpoint richiede autenticazione e ripristina anche `admin/admin` per la configurazione principale.

**Nota:** il namespace AdminSensor `remote` non viene cancellato da questo endpoint. Per disabilitare il portale remoto usare `/api/remote/reset`. Questa semantica è segnalata come candidato di hardening perché un factory reset completo potrebbe ragionevolmente includere anche lo stato remoto.

## OTA `/update`

Il GET mostra la pagina di upload dopo autenticazione.

Il POST è protetto in due punti:

1. handler finale autenticato;
2. callback che riceve i chunk autenticato **prima** di eseguire `Update.begin()`, `Update.write()` o `Update.end()`.

Questa doppia verifica evita che un client non autenticato possa iniziare a scrivere dati nella partizione OTA prima della verifica delle credenziali.

## Autenticazione locale

La baseline usa HTTP Basic e non HTTPS Web sulla LAN. L'AP di manutenzione usa:

```text
http://192.168.4.1/
```

La password factory è `admin`, modificabile dalla configurazione.

Le credenziali sono memorizzate in NVS senza cifratura applicativa; non vengono però restituite dalle API di configurazione.

## Note operative

La Web UI effettua refresh periodici tramite `/api/status`. In presenza di molti accessi consecutivi o sessioni AdminSensor, i parametri `free_heap`, `min_free_heap`, `largest_free_block` e `heap_fragmentation_pct` permettono di verificare direttamente l'effetto sulla memoria del dispositivo.

Per requisiti, trust boundary, failure mode e criteri di collaudo vedere [`ANALISI_FUNZIONALE_TECNICA.md`](ANALISI_FUNZIONALE_TECNICA.md).
