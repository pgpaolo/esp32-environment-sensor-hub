# Pinout e identificazione sensori

Questo documento descrive il pinout predefinito dell'ESP32 DevKit usato da ESP32 Environment Sensor Hub **v0.7.3**.

La dashboard espone un pulsante **PIN** su ogni card sensore. Il popup usa la configurazione runtime corrente: se un GPIO configurabile viene modificato dalla Web UI, il nuovo valore viene mostrato dopo il riavvio.

In **Diagnostica → Mappa pin** è disponibile il riepilogo completo.

## Pinout principale

| Sensore / funzione | Collegamento ESP32 | Note |
|---|---|---|
| BH1750 | SDA GPIO21, SCL GPIO22 | I2C, default `0x23` |
| BME280 | SDA GPIO21, SCL GPIO22 | I2C, `0x76/0x77` |
| DHT11 | DATA GPIO14 | ingresso digitale |
| UV analogico | ADC GPIO34 | ADC1, input-only |
| INA219 | SDA GPIO21, SCL GPIO22 | I2C, default `0x40` |
| SDS011 | ESP RX GPIO16, ESP TX GPIO17 | UART2 9600 8N1 |
| AS3935 | SDA GPIO21, SCL GPIO22, IRQ GPIO27 | I2C, `0x00..0x03`, default `0x03` |
| NESA TA-N / MAX31865 | SCK GPIO18, MISO GPIO19, MOSI GPIO23, CS GPIO13 | SPI, PT100 4 fili |
| NESA RSG1-N / ADS1115 | SDA GPIO21, SCL GPIO22 | I2C `0x48`, differenziale A0-A1 |
| Relay | GPIO15 | uscita digitale |
| LED stato | GPIO12 | uscita digitale |
| BOOT/config | GPIO0 | ingresso configurazione all'avvio |

## Bus I2C condiviso

Default:

```text
SDA = GPIO21
SCL = GPIO22
```

Indirizzi previsti:

| Dispositivo | Indirizzo |
|---|---|
| AS3935 | `0x00` - `0x03`, default `0x03` |
| BH1750 | `0x23` oppure `0x5C` |
| INA219 | `0x40` - `0x4F`, default `0x40` |
| ADS1115 / NESA RSG1-N | `0x48` - `0x4B`, default `0x48` |
| BME280 | `0x76` oppure `0x77` |

La pagina **Diagnostica** include una scansione I2C per verificare i dispositivi che rispondono realmente.

## SPI NESA TA-N

Il NESA TA-N/PT100 non viene collegato direttamente all'ESP32: usa un **MAX31865**.

```text
ESP32 GPIO18 SCK  → MAX31865 SCK/CLK
ESP32 GPIO19 MISO ← MAX31865 SDO/MISO
ESP32 GPIO23 MOSI → MAX31865 SDI/MOSI
ESP32 GPIO13 CS   → MAX31865 CS
```

Configurazione prevista:

```text
PT100 4 fili
R0   = 100 ohm
RREF = 430 ohm
```

Sono utilizzabili breakout Adafruit-compatible/DollaTek equivalenti purché configurati per PT100 con RREF circa 430 ohm. Verificare sempre serigrafia e jumper del modulo specifico.

**MAX31855 non va usato**: è un convertitore per termocoppie e non per RTD PT100.

## UART SDS011

Default:

```text
ESP32 GPIO16 (RX)  ← SDS011 TX
ESP32 GPIO17 (TX)  → SDS011 RX
```

Velocità: `9600 baud`, formato `8N1`.

L'ESP32 resta sempre acceso. Solo l'SDS011 viene posto in sleep dal firmware.

## ADC UV

Default:

```text
UV analogico → GPIO34
```

GPIO34 appartiene ad ADC1 ed è input-only. Il firmware limita il pin UV ad ADC1 GPIO32..39 per evitare l'interferenza fra ADC2 e Wi-Fi.

## GPIO con attenzione particolare

Il firmware mantiene i GPIO già usati dall'hardware esistente. Alcuni sono pin di strapping ESP32:

- GPIO0;
- GPIO12;
- GPIO15.

Su nuovi PCB evitare circuiti che alterino i livelli logici richiesti durante il boot.

GPIO6..11 sono esclusi dalla validazione perché normalmente riservati alla flash SPI dell'ESP32.

GPIO34..39 sono input-only e non possono essere usati per relay, LED, TX UART o CS SPI.

## Validazione pin

La v0.7.3 valida automaticamente i principali conflitti/usi non ammessi. In particolare:

- SDA e SCL non possono coincidere;
- RX e TX SDS011 non possono coincidere;
- pin di output non possono usare GPIO34..39;
- GPIO6..11 vengono esclusi;
- il pin UV deve appartenere ad ADC1.

Se un valore NVS non è valido, viene ripristinato il default del singolo parametro.

## Disabilitazione sensori

In **Configurazione → Sensori → Sensori attivi** ogni sensore può essere disabilitato singolarmente. La configurazione viene salvata in NVS e applicata al riavvio.

La disabilitazione non modifica il cablaggio fisico: il pulsante PIN continua a mostrare il collegamento previsto/configurato, mentre la card è indicata come `disabilitato`.
