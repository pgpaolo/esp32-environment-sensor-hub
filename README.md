# ESP32 Environment Sensor Hub

Firmware PlatformIO/Arduino per un nodo ambientale ESP32 dedicato a sensori locali, Web UI, MQTT, diagnostica e OTA. Nessuna funzione RF Oregon/Technoline è inclusa: la parte radio resta separata nel gateway dedicato.

## Versione corrente

Baseline firmware: **v0.7.2**.

Sensori supportati:

- BME280
- DHT11
- BH1750
- INA219
- UV analogico / GUVA
- SDS011
- AS3935
- NESA TA-N tramite MAX31865 / PT100 4 fili
- NESA RSG1-N tramite ADS1115 differenziale

Il firmware include inoltre relay, configurazione persistente NVS, Web UI compatta, diagnostica, MQTT JSON e aggiornamento OTA da browser.

## Web UI e accesso di manutenzione

L'ESP32 espone una Web UI protetta da autenticazione HTTP Basic.

Credenziali factory/default:

```text
user: admin
password: admin
```

Quando viene avviato l'AP di manutenzione, l'indirizzo è fissato a:

```text
http://192.168.4.1/
```

L'AP viene attivato quando non è disponibile una configurazione Wi-Fi valida, quando il collegamento STA fallisce oppure mantenendo premuto il pulsante BOOT/config all'avvio. Un factory reset ripristina anche `admin/admin`.

Le credenziali Web possono essere cambiate dalla pagina **Configurazione → Sistema → Web / Sicurezza**. Se utente o password vengono lasciati vuoti, il firmware ripristina `admin/admin`.

## Abilitazione e disabilitazione sensori

Da **Configurazione → Sensori** è presente il pannello **Sensori attivi**. Ogni sensore può essere abilitato o disabilitato singolarmente. La modifica viene applicata dopo **Salva e riavvia**.

Un sensore disabilitato:

- non viene inizializzato;
- non viene interrogato periodicamente;
- non concorre al conteggio Health;
- resta visibile in dashboard come `disabilitato`.

## Identificazione pin dalla dashboard

Ogni card sensore contiene un pulsante **PIN**. Il pulsante mostra direttamente la mappa dei collegamenti effettivi configurati sul dispositivo. In **Diagnostica** è inoltre disponibile il pulsante **Mappa pin** per visualizzare il riepilogo completo.

Vedere anche [`docs/PINOUT.md`](docs/PINOUT.md).

## Pinout ESP32 DevKit predefinito

| Funzione | GPIO |
|---|---:|
| LED stato | 12 |
| NESA TA-N / MAX31865 CS | 13 |
| DHT11 DATA | 14 |
| Relay | 15 |
| SDS011 RX ESP32 | 16 |
| SDS011 TX ESP32 | 17 |
| SPI SCK | 18 |
| SPI MISO | 19 |
| I2C SDA | 21 |
| I2C SCL | 22 |
| SPI MOSI | 23 |
| AS3935 IRQ | 27 |
| UV analogico ADC1 | 34 |
| BOOT/config | 0 |

I dispositivi I2C condividono SDA/SCL. Gli indirizzi di default sono documentati in `docs/PINOUT.md`.

## SDS011

L'ESP32 resta sempre acceso. Solo l'SDS011 viene messo in sleep per preservare laser e ventola. Il ciclo standard è:

```text
sleep → wake → warm-up → campioni → media → MQTT/Web → sleep
```

Il ciclo è configurabile dalla Web UI.

## Sensori NESA

I due sensori NESA sono predisposti ma disabilitati di default finché l'hardware non viene installato:

- **TA-N**: MAX31865, PT100 4 fili, SPI;
- **RSG1-N**: ADS1115, ingresso differenziale A0-A1, I2C.

Per dettagli vedere [`docs/NESA.md`](docs/NESA.md).

## Credenziali Wi-Fi locali

Le credenziali Wi-Fi non vengono versionate. Copiare:

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

La GitHub Action `PlatformIO Build` verifica automaticamente la build `esp32dev` a ogni push.
