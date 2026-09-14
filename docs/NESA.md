# NESA professional sensors

Il firmware è predisposto per due sensori NESA passivi, entrambi separati dalla parte RF. I due sensori sono **disabilitati di default** e possono essere attivati individualmente da **Configurazione → Sensori → Sensori attivi**.

La dashboard mostra per entrambi il pulsante **PIN**, che apre il riepilogo del collegamento corrente. La mappa completa è disponibile anche in **Diagnostica → Mappa pin**.

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

Il CS del MAX31865 è configurabile dalla Web UI. SCK/MISO/MOSI usano il bus SPI hardware standard dell'ESP32.

## RSG1-N - piranometro a termopila

Interfaccia: ADS1115 16 bit sul bus I2C condiviso.

Default:

- SDA `GPIO21`
- SCL `GPIO22`
- indirizzo ADS1115: `0x48`
- ingresso differenziale: `A0 - A1`
- gain: `GAIN_SIXTEEN` (fondo scala ±0.256 V)
- sensibilità iniziale: `10.0 µV/(W/m²)`
- fondo scala software: `2000 W/m²`

La sensibilità reale deve essere impostata usando il certificato di taratura del singolo RSG1-N. Il firmware mantiene separati valore ADC raw, millivolt e radiazione in W/m².

## Web UI

Per entrambi i sensori sono disponibili:

- checkbox di abilitazione/disabilitazione;
- parametri di taratura;
- stato operativo in dashboard;
- contatore errori in diagnostica;
- pulsante **PIN**;
- pubblicazione MQTT quando abilitati.

## Fail-safe

Un fault MAX31865 o un errore ADS1115 marca esclusivamente il relativo sensore come non valido: non viene richiesto alcun riavvio dell'ESP32.

Un sensore NESA disabilitato non viene inizializzato e non concorre al conteggio Health.
