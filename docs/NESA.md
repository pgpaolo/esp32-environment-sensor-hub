# NESA professional sensors

Il firmware è predisposto per due sensori NESA separati dalla parte RF. Entrambi sono **disabilitati di default** e possono essere attivati individualmente da **Configurazione → Sensori → Sensori attivi**.

La dashboard mostra per entrambi il pulsante **PIN**. La mappa completa è disponibile anche in **Diagnostica → Mappa pin**.

## NESA TA-N — PT100 4 fili

### Interfaccia richiesta

Il TA-N previsto da questo progetto è una **RTD PT100 a 4 fili**. Per collegarlo all'ESP32 è necessario un convertitore **MAX31865**.

Catena di acquisizione:

```text
NESA TA-N / PT100 4 fili
          ↓
       MAX31865
          ↓ SPI
         ESP32
```

Il MAX31865 non è un accessorio opzionale: fornisce l'eccitazione e la misura raziometrica della RTD, gestisce il collegamento a 4 fili, rileva fault e rende disponibile la misura via SPI.

Un **MAX31855** non è compatibile: è progettato per termocoppie, ad esempio Tipo K, non per PT100/PT1000.

### Breakout compatibile

È previsto un modulo MAX31865 Adafruit-compatible; è utilizzabile anche un breakout equivalente DollaTek purché sia configurato per:

```text
RTD  : PT100
RREF : circa 430 ohm
modo : 4 fili
```

Prima del montaggio verificare il valore della resistenza di riferimento del breakout e la configurazione/jumper prevista dal costruttore per il collegamento a 4 fili.

### Pin ESP32

| Segnale | GPIO |
|---|---:|
| SPI SCK | 18 |
| SPI MISO | 19 |
| SPI MOSI | 23 |
| MAX31865 CS | 13 |

Il firmware usa `MAX31865_4WIRE`, RTD nominale `100 ohm` e RREF `430 ohm`.

Il CS è configurabile dalla Web UI; SCK/MISO/MOSI usano il bus SPI hardware standard dell'ESP32.

### Dati esposti

- temperatura in °C;
- resistenza RTD in ohm;
- fault byte MAX31865;
- contatore errori;
- ultimo errore;
- stato Health;
- pubblicazione MQTT.

### Limiti firmware correnti

La baseline v0.7.3 è validata per **PT100** in un intervallo meteorologico. Anche se il MAX31865 può essere impiegato con PT1000, il firmware corrente non va considerato pronto per PT1000 senza adeguare nominale, RREF e controlli di plausibilità.

## NESA RSG1-N — piranometro a termopila

Interfaccia: **ADS1115 16 bit** sul bus I2C condiviso.

Catena di acquisizione:

```text
NESA RSG1-N
    ↓ segnale analogico
  ADS1115
    ↓ I2C
   ESP32
```

Default:

- SDA `GPIO21`;
- SCL `GPIO22`;
- indirizzo ADS1115 `0x48`;
- ingresso differenziale `A0-A1`;
- gain `GAIN_SIXTEEN` / fondo scala ±0,256 V;
- data rate 128 SPS;
- sensibilità iniziale `10,0 µV/(W/m²)`;
- fondo scala software `2000 W/m²`.

La sensibilità reale deve essere impostata usando il certificato di taratura del singolo RSG1-N. Il firmware mantiene separati raw ADC, millivolt e radiazione in W/m².

## Configurazione Web

Per entrambi i sensori sono disponibili:

- abilitazione/disabilitazione;
- parametri di taratura;
- stato operativo in dashboard;
- contatore errori in diagnostica;
- pulsante **PIN**;
- pubblicazione MQTT quando abilitati.

### TA-N

Parametri configurabili:

- CS MAX31865;
- RTD nominale;
- RREF;
- offset temperatura.

Default raccomandati per la baseline:

```text
CS           = GPIO13
RTD nominale = 100 ohm
RREF         = 430 ohm
offset       = 0,0 °C
```

### RSG1-N

Parametri configurabili:

- indirizzo ADS1115;
- sensibilità µV/(W/m²);
- offset µV;
- valore massimo W/m²;
- clamp dei valori negativi.

## Fail-safe

Un fault MAX31865 o un errore ADS1115 marca esclusivamente il relativo sensore come non valido: il firmware non richiede il riavvio dell'ESP32.

Un sensore NESA disabilitato non viene inizializzato, non viene campionato e non concorre al conteggio Health.

## Diagnostica

Per il TA-N vengono riportati fault MAX31865, temperatura, resistenza e contatore errori. Per il RSG1-N vengono riportati indirizzo ADS1115 rilevato, raw ADC, tensione, radiazione e contatore errori.

In caso di problemi verificare prima **Diagnostica → Mappa pin** e, per il RSG1-N, anche la scansione I2C.
