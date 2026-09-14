# Pinout e identificazione sensori

Pinout predefinito ESP32 DevKit per ESP32 Environment Sensor Hub **v0.7.3**.

Ogni card sensore espone **PIN**; **Diagnostica → Mappa pin** mostra il riepilogo completo usando la configurazione runtime corrente.

## Pinout principale

| Sensore / funzione | Collegamento ESP32 | Note |
|---|---|---|
| BH1750 | SDA GPIO21, SCL GPIO22 | I2C, default `0x23` |
| BME280 | SDA GPIO21, SCL GPIO22 | I2C, `0x76/0x77` |
| DHT11 | DATA GPIO14 | linea bidirezionale, richiede GPIO output-capable |
| UV analogico | ADC GPIO34 | ADC1, input-only |
| INA219 | SDA GPIO21, SCL GPIO22 | I2C, default `0x40` |
| SDS011 | ESP RX GPIO16, ESP TX GPIO17 | UART2 9600 8N1 |
| AS3935 | SDA GPIO21, SCL GPIO22, IRQ GPIO27 | I2C, `0x00..0x03`, default `0x03` |
| NESA TA-N / MAX31865 | SCK GPIO18, MISO GPIO19, MOSI GPIO23, CS GPIO13 | SPI, PT100 4 fili |
| NESA RSG1-N / ADS1115 | SDA GPIO21, SCL GPIO22 | I2C `0x48`, differenziale A0-A1 |
| Relay | GPIO15 | uscita digitale |
| LED stato | GPIO12 | uscita digitale |
| BOOT/config | GPIO0 | `INPUT_PULLUP`, manutenzione all'avvio |

## Bus I2C condiviso

```text
SDA = GPIO21
SCL = GPIO22
```

| Dispositivo | Indirizzo |
|---|---|
| AS3935 | `0x00..0x03`, default `0x03` |
| BH1750 | `0x23` / `0x5C` |
| INA219 | `0x40..0x4F`, default `0x40` |
| ADS1115 / RSG1-N | `0x48..0x4B`, default `0x48` |
| BME280 | `0x76` / `0x77` |

INA219 e ADS1115 hanno range potenzialmente sovrapposti. Se entrambi sono abilitati, la validazione impedisce che abbiano lo **stesso indirizzo**.

Lo scanner I2C etichetta i dispositivi noti, incluso ADS1115/RSG1-N. L'indirizzo AS3935 `0x00` non viene interrogato attivamente perché `0x00` è I2C general-call; quando usato viene segnalato nella diagnostica con una nota dedicata.

## SPI NESA TA-N

Il NESA TA-N/PT100 usa obbligatoriamente un **MAX31865**:

```text
ESP32 GPIO18 SCK  → MAX31865 SCK/CLK
ESP32 GPIO19 MISO ← MAX31865 SDO/MISO
ESP32 GPIO23 MOSI → MAX31865 SDI/MOSI
ESP32 GPIO13 CS   → MAX31865 CS
```

Baseline:

```text
PT100 4 fili
R0   = 100 ohm
RREF = 430 ohm
```

Breakout Adafruit-compatible/DollaTek equivalenti sono adatti se configurati per PT100/RREF ~430 ohm. **MAX31855 non è compatibile** perché è per termocoppie.

Il cablaggio dei quattro fili della PT100 verso i morsetti RTD del breakout dipende dal layout del modulo specifico: prima del montaggio verificare le serigrafie e la configurazione 4-wire prevista dal produttore del breakout.

## NESA RSG1-N / ADS1115

```text
ESP32 GPIO21 SDA ↔ ADS1115 SDA
ESP32 GPIO22 SCL ↔ ADS1115 SCL
RSG1-N + / -     → ADS1115 A0 / A1
```

La lettura è differenziale A0-A1. Default ADS1115 `0x48`, gain ±0,256 V.

## UART SDS011

```text
ESP32 GPIO16 (RX)  ← SDS011 TX
ESP32 GPIO17 (TX)  → SDS011 RX
```

`9600 baud`, `8N1`. L'ESP32 resta sempre acceso; solo SDS011 viene posto in sleep.

## ADC UV

```text
UV analogico → GPIO34
```

Il firmware consente il sensore UV solo su ADC1 `GPIO32..39`, evitando ADC2 durante l'uso Wi-Fi.

## GPIO con attenzione particolare

GPIO0, GPIO12 e GPIO15 sono pin di strapping: il cablaggio deve rispettare i livelli richiesti al boot.

GPIO6..11 sono esclusi perché normalmente collegati alla flash SPI interna.

GPIO34..39 sono input-only: non possono essere usati per relay, LED, SDS TX, MAX31865 CS, DHT11 o BOOT/config con pull-up interno.

## Validazione pin

La v0.7.3 verifica automaticamente:

- SDA != SCL;
- SDS RX != TX;
- pin output-capable per I2C, LED, relay, SDS TX e MAX31865 CS;
- pin bidirezionale/output-capable per DHT11;
- pin con pull-up interno utilizzabile per BOOT/config;
- UV esclusivamente su ADC1;
- esclusione GPIO6..11.

Se un valore NVS non è valido viene corretto soltanto quel parametro, senza cancellare il resto della configurazione.

## Identificazione pin dalla Web UI

Il pulsante **PIN** usa i valori correnti restituiti da `/api/status`, quindi un GPIO modificato dalla configurazione viene mostrato con il nuovo valore dopo il riavvio.

Per le periferiche con pin hardware fissi nell'attuale baseline, come SPI SCK/MISO/MOSI del MAX31865, la mappa mostra rispettivamente GPIO18/19/23.

## Disabilitazione sensori

In **Configurazione → Sensori → Sensori attivi** ogni sensore può essere disabilitato singolarmente. La disabilitazione non modifica il cablaggio fisico; il popup PIN continua a mostrare il collegamento configurato.
