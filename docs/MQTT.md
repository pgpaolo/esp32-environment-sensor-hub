# MQTT schema e diagnostica

Base topic predefinito:

```text
sensors/esp32-sensor
```

Il base topic è configurabile dalla Web UI.

## Availability / LWT

Topic:

```text
<base>/status
```

Payload retained:

```text
online
offline
```

Il client registra `offline` come Last Will retained. Dopo una connessione riuscita pubblica `online` retained sullo stesso topic.

## Telemetria

Topic:

```text
<base>/telemetry
```

Il payload è JSON. Il campo `reason` indica la causa della pubblicazione:

- `periodic` — intervallo telemetria;
- `sensor_event` — evento immediato generato da un sensore, ad esempio SDS011/AS3935.

Campi di primo livello principali:

- `device`;
- `reason`;
- `uptime_s`;
- `epoch`, quando l'orologio è valido;
- `system`;
- `relay`;
- `bh1750`;
- `bme280`;
- `dht11`;
- `ina219`;
- `nesa_ta_n`;
- `nesa_rsg1_n`;
- `uv`;
- `sds011`;
- `as3935`.

Ogni blocco sensore include almeno `enabled` e `ok`. I valori di misura vengono pubblicati quando disponibili/validi.

## Esempio ridotto

```json
{
  "device": "esp32-sensor",
  "reason": "periodic",
  "uptime_s": 86400,
  "system": {
    "rssi_dbm": -61,
    "ip": "192.168.1.221",
    "free_heap": 178432,
    "min_free_heap": 121804,
    "cpu_mhz": 240,
    "boot_count": 1,
    "mqtt": {
      "connect_attempts": 2,
      "connect_success": 1,
      "disconnects": 0,
      "publish_ok": 1440,
      "publish_failed": 0,
      "state": 0,
      "backoff_s": 5
    }
  },
  "bme280": {
    "enabled": true,
    "ok": true,
    "temperature_c": 22.4,
    "humidity_pct": 61.1,
    "pressure_hpa": 954.4,
    "dewpoint_c": 14.6
  },
  "nesa_ta_n": {
    "enabled": true,
    "ok": true,
    "temperature_c": 18.42,
    "resistance_ohm": 107.1,
    "fault": 0
  },
  "nesa_rsg1_n": {
    "enabled": true,
    "ok": true,
    "raw_adc": 876,
    "millivolts": 6.84,
    "radiation_wm2": 684.0,
    "sensitivity_uv_per_wm2": 10.0
  },
  "sds011": {
    "enabled": true,
    "ok": true,
    "state": "sleeping",
    "pm25_ugm3": 3.4,
    "pm10_ugm3": 8.1,
    "next_measurement_s": 3520
  }
}
```

## Reconnect e backoff

Il parametro base è `mqttReconnectSec`, default `5 s`.

In caso di connessione fallita il firmware raddoppia progressivamente il tempo di attesa fino a 60 secondi:

```text
5 → 10 → 20 → 40 → 60 s
```

Dopo una connessione riuscita il backoff torna al valore base configurato.

Il loop MQTT resta non bloccante rispetto al ciclo principale: fra un tentativo e l'altro il firmware continua a gestire Web UI e sensori.

## Statistiche MQTT

La diagnostica runtime mantiene:

- `connect_attempts` — tentativi di connessione;
- `connect_success` — connessioni riuscite;
- `disconnects` — transizioni osservate da connesso a disconnesso;
- `publish_ok` — publish riusciti;
- `publish_failed` — publish falliti;
- `state` / `last_state` — stato PubSubClient;
- `backoff_s` — backoff corrente;
- `last_connect_epoch`;
- `last_disconnect_epoch`;
- `last_publish_epoch`.

Le statistiche sono disponibili nella pagina **Diagnostica** e nell'endpoint `/api/status`.

## Buffer

PubSubClient usa:

```text
4096 byte
```

Il payload corrente rientra nel buffer. Non aumentare il valore senza necessità perché il buffer occupa RAM.

## TLS

Il firmware supporta MQTT plain TCP e MQTT TLS.

Opzioni:

- TLS disabilitato — `WiFiClient`;
- TLS abilitato/insecure — `WiFiClientSecure::setInsecure()`;
- TLS con CA — certificato CA configurato in NVS.

Il testo della CA già salvata non viene mai reinviato al browser. Dalla configurazione è possibile sostituirlo o cancellarlo esplicitamente.

## Credenziali

Utente e password MQTT vengono memorizzati in NVS. La password non viene restituita dall'endpoint `/api/config` e il relativo campo Web viene mostrato vuoto.

Comportamento del campo password:

```text
vuoto       → mantiene la password salvata
nuovo testo → sostituisce la password
Cancella    → rimuove la password
```

La cifratura NVS non è attiva nel progetto.

## Codici PubSubClient utili

Il codice diagnostico `state` viene esposto senza reinterpretazione. Alcuni valori frequenti:

| Stato | Significato |
|---:|---|
| 0 | connesso |
| -4 | timeout connessione |
| -2 | connessione fallita |
| 1 | protocollo MQTT non accettato |
| 2 | client ID rifiutato |
| 4 | credenziali errate |
| 5 | non autorizzato dal broker |

Per un errore persistente verificare host/porta, credenziali, ACL del broker e modalità TLS.
