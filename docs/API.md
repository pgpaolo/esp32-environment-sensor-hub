# Web UI e API HTTP

ESP32 Environment Sensor Hub v0.7.3 espone Web UI e API sulla porta TCP 80.

Tutti gli endpoint, inclusi i callback di upload OTA, sono protetti dalla stessa autenticazione HTTP Basic.

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

Le pagine statiche principali sono memorizzate in PROGMEM in `src/WebAssets.h`.

## GET `/api/status`

Restituisce lo stato runtime. Sezioni principali:

- `system`: Wi-Fi, IP, RSSI, heap, flash, firmware, reset reason, config schema;
- `mqtt`: stato, tentativi, successi, disconnessioni, publish e backoff;
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
  }
}
```

## GET `/api/config`

Restituisce i parametri necessari alla pagina configurazione ma **non** restituisce:

- password Wi-Fi;
- password MQTT;
- password Web;
- testo CA MQTT.

Espone soltanto i flag `wifi_password_set`, `mqtt_password_set`, `web_password_set` e `mqtt_ca_set`.

## GET `/api/i2c`

Restituisce lo scan testuale I2C e identifica i dispositivi noti. ADS1115/RSG1-N viene riconosciuto; AS3935 `0x00` viene segnalato come general-call ma non interrogato attivamente dallo scanner.

## POST `/api/relay/toggle`

Commuta il relay configurato.

## POST `/api/sds/measure`

Richiede una misura SDS011 fuori ciclo. Risponde `200` se accettata, `409` se lo stato corrente non lo permette.

## POST `/api/sds/sleep`

Forza SDS011 allo stato sleep. Risponde `200` in caso di successo, `409` se non applicabile.

## POST `/save`

Riceve il form, applica la validazione, salva i namespace NVS e riavvia.

```text
password vuota  → mantiene il valore corrente
nuovo valore    → sostituisce
clear flag      → cancella, ove previsto
```

Il reset credenziali Web riporta `admin/admin`.

## GET `/factory`

Cancella `sensorhub` e `sensorhub_nesa`, quindi riavvia con i default firmware. L'endpoint richiede autenticazione.

## OTA `/update`

Il GET mostra la pagina di upload dopo autenticazione.

Il POST è protetto in due punti:

1. handler finale autenticato;
2. callback che riceve i chunk autenticato **prima** di eseguire `Update.begin()`, `Update.write()` o `Update.end()`.

Questa doppia verifica evita che un client non autenticato possa iniziare a scrivere dati nella partizione OTA prima di ricevere una risposta 401.

## Autenticazione

La baseline usa HTTP Basic e non HTTPS Web. L'AP di manutenzione usa:

```text
192.168.4.1
```

La password factory è `admin`, modificabile dalla configurazione.
