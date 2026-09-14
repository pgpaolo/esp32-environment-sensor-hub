# Pinout e identificazione sensori

Questo documento descrive il pinout predefinito dell'ESP32 DevKit usato da ESP32 Environment Sensor Hub v0.7.2.

La dashboard espone un pulsante **PIN** su ogni card sensore. Il valore mostrato viene costruito usando la configurazione runtime corrente, quindi se un GPIO viene modificato dalla Web UI il popup PIN mostra il nuovo valore dopo il riavvio.

In **Diagnostica → Mappa pin** è disponibile anche il riepilogo completo.

## Pinout principale

| Sensore / funzione | Collegamento ESP32 | Note |
|---|---|---|
| BH1750 | SDA GPIO21, SCL GPIO22 | I2C, default `0x23` |
| BME280 | SDA GPIO21, SCL GPIO22 | I2C, auto-detect `0x76/0x77` |
| DHT11 | DATA GPIO14 | ingresso digitale con pull-up |
| UV analogico | ADC GPIO34 | ADC1, input-only |
| INA219 | SDA GPIO21, SCL GPIO22 | I2C, default `0x40` |
| SDS011 | ESP RX GPIO16, ESP TX GPIO17 | UART2 9600 8N1; RX ESP ← TX SDS, TX ESP → RX SDS |
| AS3935 | SDA GPIO21, SCL GPIO22, IRQ GPIO27 | I2C, auto-detect `0x00..0x03` |
| NESA TA-N / MAX31865 | SCK GPIO18, MISO GPIO19, MOSI GPIO23, CS GPIO13 | SPI, PT100 4 fili |
| NESA RSG1-N / ADS1115 | SDA GPIO21, SCL GPIO22 | I2C `0x48`, ingresso differenziale A0-A1 |
| Relay | GPIO15 | uscita digitale |
| LED stato | GPIO12 | uscita digitale |
| BOOT/config | GPIO0 | ingresso configurazione all'avvio |

## Bus I2C condiviso

Il bus I2C usa per default:

```text
SDA = GPIO21
SCL = GPIO22
```

Indirizzi previsti:

| Dispositivo | Indirizzo |
|---|---|
| AS3935 | `0x00` - `0x03`, default `0x03` |
| BH1750 | `0x23` oppure `0x5C` |
| INA219 | `0x40` |
| ADS1115 / NESA RSG1-N | `0x48` |
| BME280 | `0x76` oppure `0x77` |

La pagina **Diagnostica** include una scansione I2C per verificare quali dispositivi rispondono realmente sul bus.

## SPI NESA TA-N

Il MAX31865 usa il bus SPI hardware standard dell'ESP32:

```text
SCK  = GPIO18
MISO = GPIO19
MOSI = GPIO23
CS   = GPIO13   (configurabile)
```

Il CS può essere cambiato dalla configurazione. SCK/MISO/MOSI sono al momento associati al bus SPI hardware standard e vengono mostrati come tali nel popup PIN.

## UART SDS011

Default:

```text
ESP32 GPIO16 (RX)  ← SDS011 TX
ESP32 GPIO17 (TX)  → SDS011 RX
```

La velocità è 9600 baud, formato 8N1.

## GPIO con attenzione particolare

Il firmware mantiene i GPIO già usati dall'hardware attuale, ma alcuni sono pin di strapping ESP32:

- GPIO0
- GPIO12
- GPIO15

Il cablaggio esistente è già stato usato con Tasmota e viene quindi mantenuto. Su nuovi PCB è preferibile evitare carichi che possano alterare il livello logico di questi pin durante il boot.

GPIO34 è input-only ed è adatto alla misura analogica UV; non deve essere usato come uscita.

## Disabilitazione sensori

In **Configurazione → Sensori → Sensori attivi** ogni sensore può essere disabilitato singolarmente. La configurazione viene salvata in NVS e applicata al riavvio.

La disabilitazione non cambia il cablaggio fisico: il pulsante PIN continua a mostrare il collegamento previsto/configurato, mentre la card viene marcata come `disabilitato`.
