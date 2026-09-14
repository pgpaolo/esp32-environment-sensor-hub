# Robustezza, memoria, configurazione e MQTT

Questo documento descrive le misure introdotte in ESP32 Environment Sensor Hub **v0.7.3** per ridurre frammentazione heap, rendere la configurazione persistente aggiornabile nel tempo e migliorare la diagnostica MQTT.

## Web UI in PROGMEM

Le pagine HTML/CSS/JavaScript principali non vengono piu costruite concatenando grandi oggetti `String` in RAM.

Le pagine statiche sono memorizzate in flash tramite `PROGMEM` (`src/WebAssets.h`) e servite direttamente dal WebServer. I dati dinamici vengono caricati tramite endpoint JSON:

- `/api/status`
- `/api/config`

Anche il JSON viene serializzato direttamente sul client HTTP, evitando una seconda copia completa del payload in una `String` temporanea.

La diagnostica espone:

- heap libero corrente;
- minimo heap libero osservato dal runtime ESP32;
- largest free block;
- percentuale indicativa di frammentazione heap.

La percentuale di frammentazione e calcolata come rapporto fra heap libero totale e blocco libero contiguo piu grande. E un indicatore operativo, non una misura assoluta dell'allocatore.

## Versioning configurazione NVS

La configurazione principale usa il namespace NVS:

```text
sensorhub
```

I parametri NESA usano:

```text
sensorhub_nesa
```

Entrambi contengono una chiave schema `cfgver`.

Versione schema corrente:

```text
1
```

All'avvio, una configurazione precedente viene caricata usando i default per le chiavi mancanti, validata e poi risalvata nel formato corrente. Questo permette di aggiungere parametri nelle versioni future senza obbligare a eseguire un factory reset.

## Validazione configurazione

Prima dell'uso/salvataggio vengono verificati i principali limiti:

- porta MQTT valida;
- intervalli sensori e telemetria;
- GPIO esistenti e compatibili con input/output;
- GPIO 6..11 esclusi perche normalmente collegati alla flash ESP32;
- GPIO 34..39 esclusi dalle funzioni di output;
- UV limitato ad ADC1 GPIO32..39;
- SDA e SCL non possono coincidere;
- RX e TX SDS011 non possono coincidere;
- indirizzi I2C coerenti con i dispositivi supportati;
- limiti SDS011, UV, AS3935 e NESA.

Se un valore persistente risulta non valido viene ripristinato solo quel parametro al valore di default previsto dal firmware.

## Password nella Web UI

Le password restano memorizzate in NVS senza cifratura, per scelta progettuale.

Per evitare esposizioni inutili, il firmware **non restituisce mai** alla pagina Web:

- password Wi-Fi;
- password MQTT;
- password Web;
- testo del certificato CA MQTT gia salvato.

I campi password nella pagina Configurazione vengono quindi mostrati vuoti.

Comportamento:

```text
campo vuoto       -> mantiene il valore gia salvato
nuovo valore      -> sostituisce il valore salvato
checkbox Cancella -> cancella Wi-Fi/MQTT password o CA selezionata
```

Per le credenziali Web e disponibile anche il ripristino esplicito a:

```text
admin / admin
```

## MQTT

La connessione MQTT usa LWT retained sul topic:

```text
<base_topic>/status
```

Valori:

```text
online
offline
```

La telemetria viene pubblicata su:

```text
<base_topic>/telemetry
```

Il reconnect adotta un backoff progressivo. Partendo dal valore `mqttReconnectSec` configurato, in caso di errore il tempo raddoppia fino a un massimo di 60 secondi:

```text
5 -> 10 -> 20 -> 40 -> 60 s
```

Dopo una connessione riuscita il backoff torna al valore base configurato.

La diagnostica MQTT espone separatamente:

- tentativi di connessione;
- connessioni riuscite;
- disconnessioni osservate;
- publish riusciti;
- publish falliti;
- ultimo stato PubSubClient;
- backoff corrente;
- epoch dell'ultima connessione;
- epoch dell'ultima disconnessione;
- epoch dell'ultimo publish riuscito.

Il buffer PubSubClient resta fissato a 4096 byte, sufficiente per il payload corrente senza riservare RAM non necessaria.

## Factory reset

Il factory reset cancella entrambi i namespace NVS:

```text
sensorhub
sensorhub_nesa
```

Al riavvio vengono ricreati i valori factory del firmware, incluse le credenziali Web predefinite `admin/admin`.
