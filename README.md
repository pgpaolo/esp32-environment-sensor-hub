# ESP32 Environment Sensor Hub

Firmware PlatformIO/Arduino per un nodo ambientale ESP32 dedicato a sensori locali, Web UI, MQTT, diagnostica e OTA. Nessuna funzione RF Oregon/Technoline è inclusa: la parte radio resta separata nel gateway dedicato.

Sensori supportati nella baseline v0.7.0: BME280, DHT11, BH1750, INA219, UV analogico/GUVA, SDS011 e AS3935. Il firmware include inoltre relay, configurazione persistente, Web UI compatta, diagnostica, MQTT JSON e OTA da browser.

## Pinout ESP32 DevKit

| Funzione | GPIO |
|---|---:|
| LED | 12 |
| DHT11 | 14 |
| Relay | 15 |
| SDS011 RX | 16 |
| SDS011 TX | 17 |
| I2C SDA | 21 |
| I2C SCL | 22 |
| AS3935 IRQ | 27 |
| UV analogico | 34 |
| BOOT/config | 0 |

## SDS011

L'ESP32 resta sempre acceso. Solo l'SDS011 viene messo in sleep per preservare laser e ventola. Il ciclo è configurabile da Web UI.

## Credenziali

Le credenziali non vengono versionate. Copiare `include/DefaultSecrets.example.h` in `include/DefaultSecrets.h` e inserire i valori locali. Il file reale è escluso da Git.

## Build

```bash
pio run
pio run -t upload
pio device monitor
```

La v0.7.0 è la baseline per le prossime estensioni: INA226/power-state, MAX31865 + NESA TA-N e ADS1115 + NESA RSG1-N.
