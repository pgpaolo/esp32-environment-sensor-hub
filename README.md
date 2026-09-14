# ESP32 Environment Sensor Hub

Firmware PlatformIO/Arduino per un nodo ambientale ESP32 dedicato a sensori locali, Web UI, MQTT, diagnostica e OTA. La parte RF Oregon/Technoline **non** è inclusa in questo progetto e resta gestita dal gateway dedicato.

## Stato del progetto

Versione firmware corrente: **v0.7.3**.

La build `esp32dev` è verificata da GitHub Actions. L'ultima build funzionale della v0.7.3 ha riportato:

- RAM statica: **51.036 byte / 327.680 byte (15,6%)**;
- flash applicativa: **1.134.213 byte / 1.966.080 byte (57,7%)**;
- risultato: **SUCCESS**.

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

Il firmware include inoltre relay, configurazione persistente NVS, Web UI compatta, diagnostica, MQTT JSON, scansione I2C e aggiornamento OTA da browser.

## NESA: hardware previsto

### TA-N

Il NESA TA-N previsto dal progetto è una **PT100 a 4 fili** e richiede il front-end **MAX31865**. Il MAX31865 non viene bypassato: misura la RTD, gestisce il collegamento a 4 fili e fornisce la conversione digitale via SPI.

Configurazione firmware corrente:

```text
RTD nominale : 100 ohm
RREF         : 430 ohm
cablaggio    : 4 fili
SPI SCK      : GPIO18
SPI MISO     : GPIO19
SPI MOSI     : GPIO23
CS           : GPIO13
```

Sono adatti breakout MAX31865 per PT100, inclusi moduli Adafruit-compatible/DollaTek equivalenti, purché configurati per **PT100 / RREF ~430 ohm**. Un MAX31855 per termocoppie K **non è compatibile** con questo sensore.

### RSG1-N

Il NESA RSG1-N viene acquisito con ADS1115 in differenziale `A0-A1`, gain `±0,256 V`, indirizzo default `0x48`. La sensibilità deve essere impostata secondo il certificato di taratura del singolo piranometro.

Vedere [`docs/NESA.md`](docs/NESA.md).

## Novità v0.7.3

La v0.7.3 introduce una revisione dedicata alla robustezza:

- pagine Web statiche in `PROGMEM` (`src/WebAssets.h`), senza grandi concatenazioni `String` per dashboard/configurazione;
- JSON Web serializzato direttamente sul client HTTP;
- diagnostica memoria con heap libero, minimo heap libero, largest free block e frammentazione indicativa;
- schema NVS versionato (`cfgver`) con migrazione automatica;
- validazione dei principali parametri prima dell'uso e del salvataggio;
- statistiche MQTT separate per connect, disconnect e publish;
- reconnect MQTT con backoff progressivo fino a 60 secondi;
- password Wi-Fi, MQTT e Web mai restituite dalla Web UI;
- campo password vuoto = mantiene il valore salvato;
- pulsante **PIN** per ogni sensore e mappa pin completa in Diagnostica;
- abilitazione/disabilitazione individuale dei sensori dalla configurazione;
- AP di manutenzione fisso su `192.168.4.1`;
- autenticazione Web factory `admin / admin`.

## Web UI e accesso di manutenzione

La Web UI è protetta da autenticazione HTTP Basic.

Credenziali factory/default:

```text
user: admin
password: admin
```

Quando viene avviato l'AP di manutenzione:

```text
http://192.168.4.1/
```

L'AP viene avviato quando non è disponibile una configurazione Wi-Fi valida, quando il collegamento STA fallisce oppure mantenendo premuto il pulsante BOOT/config all'avvio. Il factory reset ripristina anche `admin/admin`.

Le password salvate a runtime restano in NVS **senza cifratura**, per scelta progettuale. Non vengono però reinviate al browser dalla pagina di configurazione.

## Abilitazione e disabilitazione sensori

In **Configurazione → Sensori → Sensori attivi** ogni sensore può essere abilitato o disabilitato singolarmente. La modifica viene applicata con **Salva e riavvia**.

Un sensore disabilitato:

- non viene inizializzato;
- non viene interrogato periodicamente;
- non concorre allo stato Health;
- resta identificabile nella dashboard/configurazione come disabilitato.

## Identificazione pin

Ogni card sensore dispone del pulsante **PIN**. Il popup usa la configurazione runtime corrente. In **Diagnostica → Mappa pin** è disponibile anche il riepilogo completo.

Vedere [`docs/PINOUT.md`](docs/PINOUT.md).

## SDS011

L'ESP32 resta sempre acceso. Solo l'SDS011 viene messo in sleep:

```text
sleep → wake → warm-up → campioni → media → MQTT/Web → sleep
```

Default: ciclo 60 min, warm-up 30 s, 5 campioni, timeout awake 120 s.

## Configurazione persistente

La configurazione principale usa il namespace NVS `sensorhub`; i parametri NESA usano `sensorhub_nesa`.

Schema corrente: **1**. Le configurazioni precedenti vengono caricate con i default per le nuove chiavi, validate e migrate automaticamente senza richiedere un factory reset.

## MQTT

Topic principali:

```text
<base_topic>/status
<base_topic>/telemetry
```

`status` usa LWT retained `online/offline`. Il reconnect adotta backoff progressivo fino a 60 s. La diagnostica espone tentativi, connessioni riuscite, disconnessioni, publish riusciti/falliti, stato PubSubClient e backoff corrente.

Vedere [`docs/MQTT.md`](docs/MQTT.md).

## Documentazione

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — architettura e flusso dati;
- [`docs/CONFIGURATION.md`](docs/CONFIGURATION.md) — configurazione, default, validazione e NVS;
- [`docs/PINOUT.md`](docs/PINOUT.md) — GPIO, bus e indirizzi;
- [`docs/NESA.md`](docs/NESA.md) — TA-N/MAX31865 e RSG1-N/ADS1115;
- [`docs/MQTT.md`](docs/MQTT.md) — topic, payload e diagnostica MQTT;
- [`docs/API.md`](docs/API.md) — endpoint Web/API;
- [`docs/ROBUSTNESS.md`](docs/ROBUSTNESS.md) — memoria, PROGMEM, versioning e fail-safe;
- [`CHANGELOG.md`](CHANGELOG.md) — cronologia delle revisioni principali.

## Credenziali Wi-Fi locali

Le credenziali factory locali non vengono versionate. Copiare:

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
