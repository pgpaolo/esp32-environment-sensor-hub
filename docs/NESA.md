# NESA professional sensors

Il firmware e predisposto per due sensori NESA passivi, entrambi separati dalla parte RF.

## TA-N - Pt100 4 fili

Interfaccia: Adafruit-compatible MAX31865 per PT100 con RREF 430 ohm.

Default ESP32:

| Segnale | GPIO |
|---|---:|
| SPI SCK | 18 |
| SPI MISO | 19 |
| SPI MOSI | 23 |
| MAX31865 CS | 13 |

Il driver usa `MAX31865_4WIRE`, RTD nominale 100 ohm e RREF 430 ohm. Sono esposti temperatura, resistenza RTD, fault byte, contatore errori e ultimo errore. Il TA-N resta passivo e non richiede alimentazione 12 V.

## RSG1-N - piranometro a termopila

Interfaccia: ADS1115 16 bit sul bus I2C condiviso.

Default:

- indirizzo ADS1115: `0x48`
- ingresso differenziale: `A0 - A1`
- gain: `GAIN_SIXTEEN` (fondo scala ±0.256 V)
- sensibilita iniziale: `10.0 uV/(W/m2)`
- fondo scala software: `2000 W/m2`

La sensibilita reale deve essere impostata usando il certificato di taratura del singolo RSG1-N. Il firmware mantiene separati valore ADC raw, millivolt e radiazione in W/m2.

## Fail-safe

I due sensori NESA sono disabilitati di default finche l'hardware non e installato. Un fault MAX31865 o un errore ADS1115 marca esclusivamente il sensore come non valido: non viene richiesto alcun riavvio dell'ESP32.
