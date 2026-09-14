# Robustezza, memoria, configurazione e MQTT

Questo documento descrive le misure presenti in ESP32 Environment Sensor Hub **v0.7.3** per ridurre frammentazione heap, rendere la configurazione persistente aggiornabile e mantenere i sensori recuperabili senza riavvii inutili.

## Build di riferimento

Build `esp32dev` verificata con PlatformIO/GitHub Actions dopo il controllo generale:

```text
RAM   51.060 / 327.680 byte  = 15,6%
Flash 1.135.421 / 1.966.080 = 57,8%
```

Questi valori sono statici/link-time. L'heap dinamico reale va controllato dalla pagina Diagnostica.

## Web UI in PROGMEM

Dashboard, configurazione e pagina OTA sono memorizzate in flash tramite `PROGMEM` in:

```text
src/WebAssets.h
```

e servite con `send_P()`.

I dati dinamici arrivano tramite:

- `/api/status`;
- `/api/config`.

Il JSON Web viene serializzato direttamente sul `WiFiClient`, evitando una seconda copia completa in una grande `String` temporanea.

## Diagnostica memoria

La Web UI espone:

- heap libero corrente;
- minimo heap libero osservato;
- largest free block;
- frammentazione indicativa;
- flash size;
- reset reason;
- firmware e schema configurazione.

La frammentazione indicativa deriva dal rapporto fra heap libero totale e blocco contiguo più grande. È un indicatore operativo, non una misura assoluta dell'allocatore.

## MQTT e allocazioni

PubSubClient usa un buffer da **4096 byte**.

Il JSON telemetrico non crea più una nuova `String` multi-kilobyte ad ogni ciclo: `MqttManager` mantiene un buffer riutilizzabile, inizialmente riservato a 3072 byte. In questo modo le pubblicazioni periodiche riducono allocazioni/deallocazioni ripetute e quindi il rischio di frammentazione dell'heap nel lungo periodo.

Reconnect progressivo con base default 5 s:

```text
5 → 10 → 20 → 40 → 60 s
```

Il backoff torna al valore base dopo una connessione riuscita.

Statistiche runtime:

- tentativi di connessione;
- connessioni riuscite;
- disconnessioni osservate;
- publish riusciti/falliti;
- stato PubSubClient;
- backoff corrente;
- epoch ultimo connect/disconnect/publish.

## Versioning configurazione NVS

Namespace:

```text
sensorhub
sensorhub_nesa
```

Schema logico corrente:

```text
cfgver = 1
```

La chiave `cfgver` viene memorizzata nel namespace principale `sensorhub`. Il namespace `sensorhub_nesa` contiene i parametri NESA ma fa parte dello stesso schema logico firmware.

Sequenza al boot:

```text
load sensorhub
  ↓
validazione / migrazione base
  ↓
load sensorhub_nesa
  ↓
seconda validazione dell'oggetto completo
  ↓
persistenza delle sole correzioni necessarie
```

La seconda validazione intercetta anche conflitti che emergono solo dopo il merge, ad esempio un indirizzo I2C INA219/ADS1115 coincidente.

## Validazione configurazione

Sono verificati almeno:

- porta MQTT e reconnect;
- intervalli sensori/telemetria;
- GPIO esistenti;
- esclusione GPIO6..11;
- GPIO34..39 esclusi dalle funzioni che richiedono output/pull-up;
- DHT su GPIO bidirezionale valido;
- BOOT/config su GPIO con `INPUT_PULLUP` utilizzabile;
- UV esclusivamente su ADC1 GPIO32..39;
- SDA != SCL e SDS RX != TX;
- indirizzi I2C dei dispositivi;
- collisione INA219/ADS1115 quando entrambi abilitati;
- enum INA219 e modalità UV;
- offset BME/DHT/BH1750/INA/NESA finiti e in range;
- limiti SDS011, UV, AS3935 e NESA;
- credenziali Web non vuote.

Un parametro non valido viene riportato al proprio default senza cancellare il resto della configurazione.

## Password e autenticazione

Le password restano in NVS senza cifratura, per scelta progettuale.

La Web UI non restituisce:

- password Wi-Fi;
- password MQTT;
- password Web;
- testo della CA MQTT salvata.

Campo vuoto = mantiene il valore; i flag espliciti consentono la cancellazione dove prevista. Il ripristino Web riporta `admin/admin`.

L'autenticazione HTTP Basic protegge dashboard, API, configurazione e OTA. Anche il **callback che riceve i chunk dell'upload OTA** verifica le credenziali prima di chiamare `Update.begin()/write()/end()`, quindi un POST non autenticato non può iniziare a scrivere il firmware.

## Fail-safe e recupero sensori

Un singolo sensore guasto non deve fermare l'ESP32.

```text
fault/read error
      ↓
contatore + last_error
      ↓
stato non OK
      ↓
retry controllato
      ↓
firmware Web/MQTT continua
```

BH1750, BME280 e INA219 tentano nuovamente l'inizializzazione quando il loro stato non è valido.

AS3935, se assente al boot, viene ritentato periodicamente senza riavvio e senza martellare il bus I2C.

### NESA TA-N

MAX31865 viene allocato una sola volta; un init fallito lascia l'interfaccia non inizializzata e viene ritentato ai cicli successivi senza `new/delete` ripetuti. I fault RTD vengono cancellati sul convertitore e riportati in diagnostica.

La baseline è **PT100 4 fili**, `R0=100 ohm`, `RREF=430 ohm`. Il MAX31865 è necessario; MAX31855 non è compatibile con questa RTD.

### NESA RSG1-N

ADS1115 viene allocato una sola volta. Se non risponde all'avvio viene ritentato. Durante il funzionamento viene verificata la presenza I2C: una disconnessione azzera lo stato rilevato e forza la reinizializzazione al ciclo successivo.

### SDS011

L'ESP32 non entra in deep sleep. Solo SDS011 usa la macchina a stati:

```text
sleep → wake → warm-up → sampling → media → publish → sleep
```

## I2C diagnostics

Lo scan I2C riconosce BH1750, BME280, INA219, ADS1115/RSG1-N e AS3935. L'indirizzo `0x00` dell'AS3935 non viene interrogato dallo scanner perché è l'indirizzo I2C general-call; se configurato viene indicato esplicitamente nella diagnostica senza effettuare una scansione attiva su `0x00`.

## Factory reset

Il factory reset cancella:

```text
sensorhub
sensorhub_nesa
```

Al riavvio vengono ricreati i default, incluse le credenziali Web `admin/admin`.

## Controlli consigliati dopo il flash

Verificare firmware/schema, heap libero/minimo/largest block, scan I2C, contatori MQTT, pin map, Health dei sensori, ciclo SDS011 e capacità di recupero dei sensori scollegati/ricollegati. La CI garantisce la compilazione, mentre questi controlli richiedono il dispositivo reale.
