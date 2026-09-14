# Web UI e API HTTP

ESP32 Environment Sensor Hub v0.7.3 espone Web UI e API sulla porta TCP 80.

Tutti gli endpoint sono protetti dalla stessa autenticazione HTTP Basic della Web UI.

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
| POST | `/update` | Upload `firmware.bin` |

Le pagine statiche principali sono memorizzate in PROGMEM in `src/WebAssets.h`.

## API stato

### GET `/api/status`

Restituisce lo stato runtime del sistema.

Sezioni principali:

- `system` — Wi-Fi, IP, RSSI, heap, flash, firmware, reset reason, config schema;
- `mqtt` — stato, tentativi, successi, disconnessioni, publish e backoff;
- `pins` — GPIO configurati/effettivi;
- `relay`;
- `bh1750`;
- `bme280`;
- `dht11`;
- `ina219`;
- `uv`;
- `nesa_ta_n`;
- `nesa_rsg1_n`;
- `sds011`;
- `as3935`.

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

## API configurazione

### GET `/api/config`

Restituisce i parametri necessari a popolare la pagina configurazione.

Per sicurezza **non** restituisce:

- password Wi-Fi;
- password MQTT;
- password Web;
- testo della CA MQTT.

Per questi valori restituisce solo flag come:

```text
wifi_password_set
mqtt_password_set
web_password_set
mqtt_ca_set
```

## Scansione I2C

### GET `/api/i2c`

Restituisce una rappresentazione testuale degli indirizzi I2C rilevati sul bus configurato.

Utile per verificare BH1750, BME280, INA219, AS3935 e ADS1115.

## Relay

### POST `/api/relay/toggle`

Commuta lo stato del relay configurato.

Risposta:

```text
OK
```

## SDS011

### POST `/api/sds/measure`

Richiede una misura SDS011 fuori ciclo.

HTTP `200` quando la richiesta viene accettata; `409` se lo stato corrente non consente di avviare la misura.

### POST `/api/sds/sleep`

Forza l'SDS011 allo stato sleep.

HTTP `200` in caso di successo; `409` se l'operazione non può essere eseguita.

## Salvataggio configurazione

### POST `/save`

Riceve i campi del form configurazione, applica validazione, salva in NVS e riavvia l'ESP32.

Le password seguono la logica:

```text
campo vuoto  → mantiene il valore corrente
nuovo valore → sostituisce
clear flag   → cancella, ove previsto
```

Il ripristino delle credenziali Web riporta `admin/admin`.

## Factory reset

### GET `/factory`

Cancella i namespace NVS:

```text
sensorhub
sensorhub_nesa
```

e riavvia il dispositivo con i default firmware.

## Autenticazione

L'autenticazione è HTTP Basic. Non è previsto HTTPS per la Web UI nella baseline v0.7.3.

L'AP di manutenzione usa:

```text
192.168.4.1
```

La password Web default resta `admin`, ma può essere modificata dalla configurazione.
