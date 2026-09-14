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

Il payload è JSON. `reason` vale normalmente `periodic` oppure `sensor_event` per pubblicazioni immediate da sensore/evento.

Blocchi principali:

```text
device
reason
uptime_s
epoch
system
relay
bh1750
bme280
dht11
ina219
nesa_ta_n
nesa_rsg1_n
uv
sds011
as3935
```

Ogni blocco sensore include `enabled` e `ok`; i valori vengono pubblicati quando disponibili/validi.

## Reconnect e backoff

Il parametro base `mqttReconnectSec` è 5 s per default e viene validato fra 1 e 300 s.

Dopo fallimenti consecutivi il firmware applica:

```text
5 → 10 → 20 → 40 → 60 s
```

Il backoff torna al valore base alla prima connessione riuscita. Fra i tentativi il loop continua a gestire Web UI e sensori.

## Statistiche MQTT

La diagnostica mantiene:

- `connect_attempts`;
- `connect_success`;
- `disconnects`;
- `publish_ok`;
- `publish_failed`;
- `state` / `last_state`;
- `backoff_s`;
- `last_connect_epoch`;
- `last_disconnect_epoch`;
- `last_publish_epoch`.

Sono disponibili in **Diagnostica**, `/api/status` e in parte nel blocco `system.mqtt` della telemetria.

## Gestione memoria MQTT

PubSubClient usa:

```text
4096 byte
```

Il payload corrente rientra nel buffer.

Per ridurre la frammentazione dell'heap, `MqttManager` non costruisce più una nuova `String` multi-kilobyte ad ogni pubblicazione: mantiene un buffer membro riutilizzato fra i cicli e riservato inizialmente a 3072 byte. Ad ogni publish viene azzerata la lunghezza logica, mantenendo la capacità allocata quando possibile.

Questo intervento è complementare alla Web UI in PROGMEM e alla diagnostica `free_heap/min_free_heap/largest_free_block`.

## TLS

Sono supportati:

- MQTT plain TCP con `WiFiClient`;
- TLS insecure con `WiFiClientSecure::setInsecure()`;
- TLS con CA salvata in NVS.

Il testo della CA già salvata non viene reinviato al browser. Può essere sostituito o cancellato esplicitamente dalla configurazione.

## Credenziali

Utente e password MQTT vengono memorizzati in NVS **senza cifratura**, per scelta progettuale. La password non viene restituita da `/api/config` e il campo Web è sempre vuoto.

```text
campo vuoto  → mantiene la password salvata
nuovo testo  → sostituisce la password
Cancella     → rimuove la password
```

## Codici PubSubClient utili

| Stato | Significato |
|---:|---|
| 0 | connesso |
| -4 | timeout connessione |
| -2 | connessione fallita |
| 1 | protocollo MQTT non accettato |
| 2 | client ID rifiutato |
| 4 | credenziali errate |
| 5 | non autorizzato dal broker |

Per errori persistenti verificare host/porta, credenziali, ACL e modalità TLS.
