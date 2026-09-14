# Robustezza, memoria, configurazione e MQTT

Questo documento descrive le misure presenti in ESP32 Environment Sensor Hub **v0.7.3** per ridurre frammentazione heap, rendere la configurazione persistente aggiornabile nel tempo e migliorare la diagnostica.

## Build di riferimento

Build `esp32dev` verificata con PlatformIO/GitHub Actions:

```text
RAM   51.036 / 327.680 byte  = 15,6%
Flash 1.134.213 / 1.966.080 = 57,7%
```

Questi valori sono riferiti all'occupazione statica/link-time. L'heap dinamico reale va controllato sul dispositivo dalla pagina Diagnostica.

## Web UI in PROGMEM

Le pagine HTML/CSS/JavaScript principali non vengono più costruite concatenando grandi oggetti `String` in RAM.

Gli asset statici sono memorizzati in flash tramite `PROGMEM` in:

```text
src/WebAssets.h
```

e serviti con `send_P()`.

I dati dinamici vengono caricati tramite JSON:

- `/api/status`;
- `/api/config`.

Il JSON Web viene serializzato direttamente sul `WiFiClient`, evitando una seconda copia completa del documento in una `String` temporanea.

## Diagnostica memoria

La Web UI espone:

- heap libero corrente;
- minimo heap libero osservato;
- largest free block;
- frammentazione indicativa;
- flash size;
- reset reason;
- firmware e schema configurazione.

La frammentazione indicativa è calcolata da heap libero totale e blocco contiguo più grande. È un indicatore operativo, non una misura assoluta dell'allocatore.

Per valutare stabilità nel tempo osservare soprattutto:

1. `min_free_heap`;
2. `largest_free_block`;
3. andamento della frammentazione dopo molti accessi Web/MQTT.

## Versioning configurazione NVS

Namespace principali:

```text
sensorhub
sensorhub_nesa
```

Entrambi usano la chiave schema:

```text
cfgver
```

Versione schema corrente:

```text
1
```

All'avvio, una configurazione precedente viene caricata usando i default per le chiavi mancanti, validata e risalvata nel formato corrente quando necessario.

Questo permette di aggiungere parametri futuri senza obbligare a un factory reset.

## Validazione configurazione

Prima dell'uso/salvataggio vengono verificati i principali limiti:

- porta MQTT valida;
- reconnect MQTT 1..300 s;
- intervallo sensori 2..86400 s;
- telemetria 5..86400 s;
- GPIO esistenti;
- GPIO6..11 esclusi perché normalmente collegati alla flash;
- GPIO34..39 esclusi dalle funzioni di output;
- UV limitato ad ADC1 GPIO32..39;
- SDA e SCL differenti;
- RX e TX SDS011 differenti;
- indirizzi I2C coerenti con i dispositivi supportati;
- limiti SDS011, UV, AS3935 e NESA;
- credenziali Web non vuote.

Se un valore persistente risulta non valido viene ripristinato **solo quel parametro** al default firmware.

## Password nella Web UI

Le password restano memorizzate in NVS senza cifratura, per scelta progettuale.

Il firmware non restituisce alla pagina Web:

- password Wi-Fi;
- password MQTT;
- password Web;
- testo della CA MQTT già salvata.

Comportamento:

```text
campo vuoto       → mantiene il valore già salvato
nuovo valore      → sostituisce il valore salvato
checkbox Cancella → cancella Wi-Fi/MQTT password o CA selezionata
```

Per le credenziali Web è disponibile il ripristino esplicito:

```text
admin / admin
```

L'autenticazione è HTTP Basic: limita l'accesso alla UI ma non cifra il traffico HTTP. Sul progetto corrente non è previsto HTTPS Web.

## MQTT

Topic availability:

```text
<base_topic>/status
```

Valori retained:

```text
online
offline
```

Telemetria:

```text
<base_topic>/telemetry
```

Reconnect progressivo, partendo da `mqttReconnectSec`:

```text
5 → 10 → 20 → 40 → 60 s
```

Il backoff torna al valore base dopo una connessione riuscita.

Statistiche esposte:

- tentativi di connessione;
- connessioni riuscite;
- disconnessioni osservate;
- publish riusciti;
- publish falliti;
- ultimo stato PubSubClient;
- backoff corrente;
- epoch di ultimo connect/disconnect/publish.

Il buffer PubSubClient è fissato a **4096 byte**.

## Sensori e fail-safe

Il principio generale è che un singolo sensore guasto non deve fermare l'ESP32.

Un sensore non valido:

```text
fault / read error
      ↓
contatore errori
      ↓
last_error
      ↓
card/Health non OK
      ↓
firmware continua a funzionare
```

I sensori disabilitati non vengono inizializzati e non concorrono allo stato Health.

### SDS011

L'ESP32 non entra in deep sleep. Solo SDS011 viene gestito a stati:

```text
sleep → wake → warm-up → sampling → media → publish → sleep
```

### NESA TA-N

Un fault MAX31865 viene registrato, cancellato sul convertitore e riportato in diagnostica senza riavviare l'ESP32.

### NESA RSG1-N

Un ADS1115 non disponibile o un valore di radiazione implausibile rende non valido solo il relativo sensore.

## Factory reset

Il factory reset cancella:

```text
sensorhub
sensorhub_nesa
```

Al riavvio vengono ricreati i default, incluse le credenziali Web `admin/admin`.

## Controlli consigliati dopo un aggiornamento

Dopo un OTA o una variazione importante della configurazione verificare:

- firmware e schema configurazione in Diagnostica;
- heap libero/minimo/largest block;
- scansione I2C;
- stato MQTT e contatori;
- pin map;
- Health dei sensori abilitati;
- SDS011 che ritorni regolarmente allo stato `sleeping`.
