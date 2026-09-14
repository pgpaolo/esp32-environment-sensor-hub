# NESA professional sensors

Il firmware è predisposto per due sensori NESA separati dalla parte RF. Entrambi sono **disabilitati di default** e possono essere attivati individualmente da **Configurazione → Sensori → Sensori attivi**.

La dashboard mostra per entrambi il pulsante **PIN**. La mappa completa è disponibile anche in **Diagnostica → Mappa pin**.

## NESA TA-N — PT100 4 fili

Il TA-N previsto da questo progetto è una **RTD PT100 a 4 fili** e richiede un convertitore **MAX31865**.

```text
NESA TA-N / PT100 4 fili
          ↓
       MAX31865
          ↓ SPI
         ESP32
```

Il MAX31865 non è opzionale: fornisce eccitazione e misura raziometrica della RTD, gestisce il collegamento 4 fili, rileva fault e rende la misura disponibile via SPI.

Un **MAX31855** non è compatibile: è un convertitore per termocoppie, ad esempio Tipo K, non per PT100/PT1000.

### Breakout compatibile

È previsto un MAX31865 Adafruit-compatible; va bene anche un equivalente DollaTek se configurato per:

```text
RTD  : PT100
RREF : circa 430 ohm
modo : 4 fili
```

Prima del montaggio verificare la resistenza di riferimento e i jumper/bridge previsti dal modulo specifico.

### Pin ESP32

| Segnale | GPIO |
|---|---:|
| SPI SCK | 18 |
| SPI MISO | 19 |
| SPI MOSI | 23 |
| MAX31865 CS | 13 |

Il firmware usa `MAX31865_4WIRE`, RTD nominale `100 ohm` e RREF `430 ohm`.

### Dati esposti

- temperatura °C;
- resistenza RTD ohm;
- fault byte MAX31865;
- contatore errori;
- ultimo errore;
- stato Health;
- pubblicazione MQTT.

### Recupero automatico

L'oggetto MAX31865 viene allocato una sola volta. Se `begin()` fallisce, il sensore viene marcato non valido ma il firmware continua; ai successivi cicli sensori viene ritentata l'inizializzazione sullo stesso oggetto, evitando churn `new/delete` dell'heap.

Un fault RTD durante il funzionamento viene registrato e cancellato dal MAX31865, senza riavviare l'ESP32.

### Limiti firmware correnti

La baseline v0.7.3 è validata per **PT100** in intervallo meteorologico. Anche se il MAX31865 supporta PT1000, il firmware corrente non va considerato PT1000-ready senza adeguare nominale, RREF e plausibilità della resistenza.

## NESA RSG1-N — piranometro a termopila

Interfaccia: **ADS1115 16 bit** sul bus I2C condiviso.

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
- ADS1115 `0x48`;
- differenziale `A0-A1`;
- `GAIN_SIXTEEN` / ±0,256 V;
- 128 SPS;
- sensibilità iniziale `10,0 µV/(W/m²)`;
- massimo software `2000 W/m²`.

La sensibilità reale deve essere sostituita con quella del certificato di taratura del singolo RSG1-N.

### Recupero automatico

L'ADS1115 viene allocato una sola volta. Se non risponde all'avvio, l'inizializzazione viene ritentata ai cicli successivi. Prima di ogni campionamento viene verificata la presenza all'indirizzo configurato: se il modulo viene scollegato, lo stato passa a non valido, l'indirizzo rilevato torna `0xFF` e il firmware tenterà la reinizializzazione al ciclo seguente.

Questo permette il recupero dopo riconnessione senza reboot del nodo.

## Configurazione Web

Per entrambi i sensori sono disponibili abilitazione/disabilitazione, parametri di taratura, stato dashboard, contatore errori, pulsante **PIN** e pubblicazione MQTT.

### TA-N

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
- massimo W/m²;
- clamp valori negativi.

Il range ADS1115 ammesso è `0x48..0x4B`. Se INA219 e ADS1115 sono entrambi abilitati, la validazione impedisce che usino lo stesso indirizzo I2C.

## Fail-safe

Un fault MAX31865 o un errore/disconnessione ADS1115 marca esclusivamente il relativo sensore come non valido. L'ESP32, Web UI, MQTT e gli altri sensori continuano a funzionare.

Un sensore NESA disabilitato non viene inizializzato né campionato e non concorre al conteggio Health.

## Diagnostica

Per TA-N vengono riportati fault, temperatura, resistenza e contatore errori. Per RSG1-N vengono riportati indirizzo ADS1115 rilevato, raw ADC, tensione, radiazione e contatore errori.

Lo scan I2C riconosce l'ADS1115/RSG1-N all'indirizzo configurato.
