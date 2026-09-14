# ESP32 Environment Sensor Hub

Firmware PlatformIO/Arduino per un nodo ambientale ESP32 dedicato a sensori locali, Web UI, MQTT, diagnostica e OTA. La parte RF Oregon/Technoline **non** è inclusa in questo progetto e resta gestita dal gateway dedicato.

## Stato del progetto

Versione firmware corrente: **v0.7.3**.

Controllo generale completato sul branch `main`: build PlatformIO `esp32dev` **SUCCESS** sull'HEAD verificato.

- RAM statica: **51.060 / 327.680 byte (15,6%)**;
- flash applicativa: **1.135.421 / 1.966.080 byte (57,8%)**.

La CI compila automaticamente il progetto ad ogni push.

## Sensori supportati

| Sensore | Interfaccia | Stato default |
|---|---|---|
| BME280 | I2C | abilitato |
| DHT11 | GPIO | abilitato |
| BH1750 | I2C | abilitato |
| INA219 | I2C | abilitato |
| UV analogico / GUVA | ADC1 | abilitato |
| SDS011 | UART2 | abilitato |
| AS3935 | I2C + IRQ | abilitato |
| NESA TA-N / PT100 | MAX31865 + SPI | disabilitato |
| NESA RSG1-N | ADS1115 + I2C | disabilitato |

Il firmware include relay, configurazione persistente NVS, Web UI compatta, diagnostica, MQTT JSON, scansione I2C e aggiornamento OTA da browser.

## NESA: hardware previsto

### TA-N

Il NESA TA-N previsto dal progetto è una **PT100 a 4 fili** e richiede il front-end **MAX31865**. Il MAX31865 non viene bypassato: misura la RTD, gestisce il collegamento a 4 fili e fornisce la conversione digitale via SPI.

```text
RTD nominale : 100 ohm
RREF         : 430 ohm
cablaggio    : 4 fili
SPI SCK      : GPIO18
SPI MISO     : GPIO19
SPI MOSI     : GPIO23
CS           : GPIO13
```

Sono adatti breakout MAX31865 per PT100, inclusi moduli Adafruit-compatible/DollaTek equivalenti, purché configurati per **PT100 / RREF ~430 ohm**. Un MAX31855 per termocoppie K **non è compatibile**.

### RSG1-N

Il NESA RSG1-N viene acquisito con ADS1115 in differenziale `A0-A1`, gain `±0,256 V`, indirizzo default `0x48`. La sensibilità deve essere impostata secondo il certificato di taratura del singolo piranometro.

Entrambe le interfacce NESA tentano il recupero automatico dopo un errore di inizializzazione senza richiedere il riavvio dell'ESP32. L'ADS1115 viene inoltre controllato durante il funzionamento: una disconnessione viene rilevata e il driver viene reinizializzato al ciclo successivo.

Vedere [`docs/NESA.md`](docs/NESA.md) e [`docs/SENSORI.md`](docs/SENSORI.md).

## Robustezza v0.7.3

La v0.7.3 include una revisione completa di memoria, configurazione, MQTT e fail-safe:

- pagine Web statiche in `PROGMEM` (`src/WebAssets.h`);
- JSON Web serializzato direttamente sul client HTTP;
- diagnostica heap: libero, minimo, largest free block e frammentazione indicativa;
- buffer JSON MQTT riutilizzato fra le pubblicazioni per ridurre churn/frammentazione heap;
- schema NVS versionato (`cfgver = 1`) nel namespace principale e validazione finale dell'oggetto configurazione dopo il caricamento del namespace NESA;
- validazione di GPIO, enum, offset, indirizzi I2C, intervalli e collisione INA219/ADS1115;
- MQTT con LWT retained, statistiche separate e backoff fino a 60 s;
- password Wi-Fi/MQTT/Web e CA MQTT mai restituite dalla Web UI;
- OTA protetto da autenticazione anche durante il callback di upload, prima della scrittura dei chunk firmware;
- recupero automatico di BH1750/BME280/INA219, NESA e AS3935 senza riavvio generale;
- pulsante **PIN** per ogni sensore e mappa pin completa;
- abilitazione/disabilitazione individuale dei sensori;
- AP manutenzione `192.168.4.1` e autenticazione factory `admin/admin`.

## Web UI e accesso di manutenzione

La Web UI è protetta da autenticazione HTTP Basic.

```text
user: admin
password: admin
```

AP manutenzione:

```text
http://192.168.4.1/
```

L'AP viene avviato se manca una configurazione Wi-Fi valida, se il collegamento STA fallisce o mantenendo premuto BOOT/config all'avvio. Il factory reset ripristina anche `admin/admin`.

Le password restano memorizzate in NVS **senza cifratura**, per scelta progettuale, ma non vengono mai reinviate al browser.

## Sensori attivi e pin

In **Configurazione → Sensori → Sensori attivi** ogni sensore può essere abilitato/disabilitato singolarmente. Un sensore disabilitato non viene inizializzato, non viene interrogato e non concorre allo stato Health.

Ogni card dispone del pulsante **PIN**; **Diagnostica → Mappa pin** mostra il riepilogo completo usando i valori runtime correnti.

Vedere [`docs/PINOUT.md`](docs/PINOUT.md).

## SDS011

L'ESP32 resta sempre acceso. Solo SDS011 entra in sleep:

```text
sleep → wake → warm-up → campioni → media → MQTT/Web → sleep
```

Default: ciclo 60 min, warm-up 30 s, 5 campioni, timeout awake 120 s.

## Configurazione persistente

Namespace NVS:

```text
sensorhub
sensorhub_nesa
```

Lo schema logico corrente è **1**. La chiave `cfgver` è salvata nel namespace principale `sensorhub`; dopo il caricamento dei parametri NESA viene eseguita una seconda validazione dell'oggetto completo e le eventuali correzioni vengono persistite nei rispettivi namespace.

## MQTT

Topic principali:

```text
<base_topic>/status
<base_topic>/telemetry
```

`status` usa LWT retained `online/offline`. Il reconnect usa backoff progressivo fino a 60 s. Il buffer PubSubClient resta 4096 byte; il payload JSON applicativo viene costruito in un buffer `String` riutilizzato fra i cicli per evitare continue allocazioni di qualche KB.

Vedere [`docs/MQTT.md`](docs/MQTT.md).

## Documentazione

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — architettura e flusso dati;
- [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) — configurazione, default, validazione e NVS;
- [`docs/PINOUT.md`](docs/PINOUT.md) — GPIO, bus e indirizzi;
- [`docs/SENSORI.md`](docs/SENSORI.md) — caratteristiche tecniche di tutti i sensori, con approfondimento NESA;
- [`docs/NESA.md`](docs/NESA.md) — TA-N/MAX31865 e RSG1-N/ADS1115;
- [`docs/MQTT.md`](docs/MQTT.md) — topic, payload e diagnostica MQTT;
- [`docs/API.md`](docs/API.md) — endpoint Web/API e OTA;
- [`docs/ROBUSTNESS.md`](docs/ROBUSTNESS.md) — memoria, PROGMEM, versioning e fail-safe;
- [`docs/VERIFICATION.md`](docs/VERIFICATION.md) — checklist del controllo generale v0.7.3;
- [`CHANGELOG.md`](CHANGELOG.md) — cronologia delle revisioni principali.

## Credenziali Wi-Fi locali

Le credenziali locali non vengono versionate. Copiare:

```text
include/DefaultSecrets.example.h
```

in:

```text
include/DefaultSecrets.h
```

e inserire i valori locali. Il file reale è escluso da Git.

## Build

```bash
pio run
pio run -t upload
pio device monitor
```

La GitHub Action `PlatformIO Build` verifica automaticamente `pio run -e esp32dev` a ogni push.
