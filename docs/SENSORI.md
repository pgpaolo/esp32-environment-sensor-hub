# Caratteristiche dei sensori

Documento tecnico di riferimento per **ESP32 Environment Sensor Hub**.

Scopo: raccogliere in un unico punto le caratteristiche dei sensori gestiti dal firmware, distinguendo chiaramente fra:

- **caratteristiche del sensore / componente** dichiarate dal costruttore;
- **modalità di acquisizione adottata dal progetto**;
- **parametri di configurazione firmware**;
- **note di installazione e taratura**.

> **Nota importante** — I valori riportati dai datasheet descrivono i singoli sensori o convertitori. L'accuratezza complessiva del sistema dipende anche da cablaggio, alimentazione, front-end, taratura, schermatura, installazione, condizioni ambientali e certificato individuale del sensore. Per i NESA, in particolare per il piranometro RSG1-N, la costante riportata sul certificato di taratura del singolo strumento prevale sempre sul valore nominale usato come default firmware.

---

## 1. Quadro generale

| Sensore | Grandezza | Interfaccia progetto | Default | Collegamento ESP32 |
|---|---|---|---|---|
| BME280 | temperatura, umidità, pressione | I2C | ON | SDA 21 / SCL 22, `0x76` |
| DHT11 | temperatura, umidità | GPIO digitale | ON | DATA GPIO14 |
| BH1750 | illuminamento | I2C | ON | SDA 21 / SCL 22, `0x23` |
| INA219 | tensione, corrente, potenza | I2C | ON | SDA 21 / SCL 22, `0x40` |
| UV analogico / GUVA | indice UV | ADC1 | ON | GPIO34 |
| SDS011 | PM2.5, PM10 | UART2 | ON | RX16 / TX17 |
| AS3935 | attività elettrica atmosferica | I2C + IRQ | ON | SDA21 / SCL22 / IRQ27 |
| **NESA TA-N** | **temperatura aria** | **PT100 4 fili → MAX31865 → SPI** | OFF | SCK18 / MISO19 / MOSI23 / CS13 |
| **NESA RSG1-N** | **radiazione solare globale** | **termopila → ADS1115 → I2C** | OFF | SDA21 / SCL22, ADS1115 `0x48` |

I sensori possono essere abilitati o disabilitati singolarmente dalla pagina di configurazione. Un sensore disabilitato non viene inizializzato, non viene campionato e non concorre allo stato Health.

---

# 2. Sensori NESA

I due sensori NESA rappresentano la parte metrologicamente più significativa del progetto. Il firmware li mantiene separati dai sensori consumer e usa front-end dedicati, scelti in funzione del tipo di trasduttore.

## 2.1 NESA TA-N — temperatura aria, PT100 1/3 DIN a 4 fili

### Funzione

Il **TA-N** è la versione a uscita naturale/resistiva del sensore NESA per temperatura dell'aria. Il trasduttore è una **termoresistenza al platino PT100 1/3 DIN** con collegamento a **quattro fili**.

Il sensore è protetto da uno schermo contro radiazione solare diretta e UV, progettato per consentire ventilazione naturale. La documentazione NESA indica conformità WMO e EN 15518-3:2011 per la famiglia TA/TAV.

### Caratteristiche dichiarate da NESA

| Parametro | Valore |
|---|---|
| Trasduttore | PT100 al platino, 1/3 DIN |
| Resistenza nominale | `100 Ω @ 0 °C` |
| Collegamento TA-N | 4 fili |
| Campo tipico | `-40 … +60 °C` |
| Altri range | disponibili su richiesta |
| Risoluzione indicata | `0,01 °C` |
| Accuratezza elemento | DIN 43760 1/3 DIN, circa `±0,1 °C @ 0 °C` |
| Tempo di risposta | `< 10 s` |
| Temperatura di lavoro | `-60 … +80 °C` |
| Ventilazione | naturale, famiglia TA |
| Costruzione | lega di alluminio, viteria inox |
| Peso indicativo | circa 700 g |
| Uscita versione N | PT100 resistiva naturale |

### Perché serve il MAX31865

Il TA-N **non produce una tensione già proporzionale alla temperatura**: la grandezza fisica disponibile è la resistenza della PT100. L'ESP32 non è quindi collegato direttamente al sensore.

Catena prevista:

```text
NESA TA-N
PT100 1/3 DIN - 4 fili
        │
        ▼
     MAX31865
  RTD-to-digital
        │ SPI
        ▼
      ESP32
```

Il **MAX31865** è un convertitore dedicato a RTD al platino. Supporta PT100…PT1000, collegamenti 2/3/4 fili, interfaccia SPI e diagnostica di circuito aperto/corto.

Per il nostro progetto:

```text
RTD nominale     100 Ω
RREF             430 Ω
modo             4 fili
SPI SCK          GPIO18
SPI MISO         GPIO19
SPI MOSI         GPIO23
CS               GPIO13
```

Il firmware usa `MAX31865_4WIRE`.

### MAX31865: caratteristiche utili alla catena di misura

Secondo Analog Devices / Maxim:

- RTD supportate: da **100 Ω a 1 kΩ a 0 °C**;
- collegamenti supportati: 2, 3 e 4 fili;
- interfaccia SPI;
- ADC interno a 15 bit;
- risoluzione nominale di temperatura del convertitore circa `0,03125 °C` (dipendente dalla non linearità RTD);
- accuratezza del convertitore fino a circa `0,5 °C` worst-case nelle condizioni indicate dal costruttore;
- diagnostica per RTD/cavo aperto e corto;
- resistenza esterna di riferimento necessaria per impostare correttamente la misura.

> L'accuratezza della **PT100 NESA** e quella del **MAX31865** non vanno sommate o confuse come se fossero la stessa specifica. La precisione reale della catena dipende anche dalla tolleranza e dal coefficiente termico della RREF del breakout, dal cablaggio e dalla calibrazione.

### Perché 4 fili

Il collegamento a quattro fili consente di ridurre l'errore dovuto alla resistenza dei cavi. È la configurazione preferibile quando il sensore è installato all'esterno e il cavo può essere lungo diversi metri.

### Breakout ammesso

È adatto un breakout **MAX31865 per PT100** Adafruit-compatible o equivalente, incluso il modulo DollaTek discusso per il progetto, purché:

- sia effettivamente configurato per **PT100**;
- utilizzi una **RREF circa 430 Ω**;
- sia configurato per il collegamento **4 fili**;
- i livelli logici lato ESP32 siano compatibili con 3,3 V.

Un **MAX31855** non è utilizzabile per questo sensore: il MAX31855 è destinato alle **termocoppie**, non alle RTD PT100/PT1000.

### Configurazione firmware

Default del progetto:

```text
NESA TA-N enabled    false
CS                   GPIO13
RTD nominale         100.0 Ω
RREF                 430.0 Ω
offset temperatura   0.0 °C
```

Dati esposti da Web/MQTT:

- temperatura in °C;
- resistenza RTD in Ω;
- fault MAX31865;
- contatore errori;
- ultimo errore;
- stato `enabled` / `ok`.

### Fail-safe

Un fault RTD non deve riavviare l'ESP32. Il firmware:

1. marca il TA-N non valido;
2. registra fault e contatore;
3. cancella il fault sul MAX31865;
4. mantiene attivi Web, MQTT e gli altri sensori;
5. ritenta l'inizializzazione nei cicli successivi se necessario.

---

## 2.2 NESA RSG1-N — piranometro a termopila

### Funzione

Il **RSG1-N** misura la **radiazione solare globale**. È un piranometro a **termopila**: la radiazione incidente riscalda la superficie assorbente del trasduttore e genera un piccolo segnale elettrico proporzionale all'irradianza.

La versione **N** fornisce l'uscita naturale della termopila e non richiede una conversione interna 0-2 V, 4-20 mA o Modbus.

### Caratteristiche dichiarate da NESA

La scheda tecnica NESA corrente descrive RSG1 come piranometro **I Classe / Classe B**, conforme ISO 9060 e WMO.

| Parametro | Valore |
|---|---|
| Principio | termopila |
| Grandezza | radiazione solare globale |
| Classe | I Classe / Classe B, secondo documentazione NESA |
| Campo di misura | `0 … 2000 W/m²` |
| Campo spettrale | `0,3 … 3 µm` |
| Sensibilità tipica | circa `10 µV/(W/m²)` |
| Tempo di risposta | `< 20 s` |
| Stabilità a lungo termine | `< ±1,5 %` |
| Incertezza giornaliera attesa | `< 5 %` |
| Impedenza di uscita | `< 40 Ω` |
| Condizioni operative | `-40 … +80 °C` |
| Corpo | alluminio, protezione IP67 |
| Peso | `< 630 g` |
| Uscita RSG1-N | segnale naturale della termopila |
| Taratura | certificato con costante individuale |

La documentazione NESA specifica che ogni strumento viene tarato per confronto con uno strumento campione e viene fornito con la relativa **costante strumentale**.

### Punto fondamentale: usare la sensibilità del certificato

Nel firmware il default è:

```text
10.0 µV/(W/m²)
```

Questo è solo il **valore nominale tipico**. Dopo l'installazione del sensore deve essere inserita la sensibilità riportata sul **certificato di taratura del proprio RSG1-N**.

Esempio:

```text
certificato sensore = 9.73 µV/(W/m²)
```

La configurazione firmware deve diventare:

```text
sensitivity = 9.73
```

La conversione usata è:

```text
Radiazione [W/m²] = (Vout [µV] - Offset [µV]) / Sensibilità [µV/(W/m²)]
```

### Perché viene usato ADS1115

Il segnale del RSG1-N è dell'ordine dei **microvolt per W/m²**. Con sensibilità nominale di 10 µV/(W/m²), a 1000 W/m² il segnale è circa:

```text
10 mV
```

e a 2000 W/m² circa:

```text
20 mV
```

Per questo il progetto non usa direttamente l'ADC interno dell'ESP32 ma un **ADS1115 a 16 bit**, in misura differenziale.

Catena prevista:

```text
NESA RSG1-N
termopila
   │ segnale naturale in mV
   ▼
ADS1115
A0 - A1 differenziale
GAIN_SIXTEEN
FSR ±0,256 V
   │ I2C
   ▼
ESP32
```

### Configurazione ADS1115 nel progetto

```text
indirizzo I2C       0x48
canale              A0-A1 differenziale
gain                GAIN_SIXTEEN
full scale          ±0,256 V
data rate           128 SPS
SDA                 GPIO21
SCL                 GPIO22
```

Con FSR `±0,256 V`, l'ADS1115 ha un passo nominale di circa:

```text
7,8125 µV/LSB
```

Con un RSG1-N nominale da `10 µV/(W/m²)`, ciò corrisponde teoricamente a circa:

```text
0,781 W/m² per LSB
```

Questo valore descrive la **quantizzazione teorica**, non l'accuratezza assoluta del sistema. Rumore, offset, guadagno, cablaggio e costante di taratura restano determinanti.

L'impedenza differenziale dell'ADS1115 nel range ±0,256 V è molto maggiore dell'impedenza d'uscita dichiarata del RSG1-N; il carico introdotto dall'ADC è quindi trascurabile ai fini pratici della catena prevista.

### Configurazione firmware

Default:

```text
NESA RSG1-N enabled        false
ADS1115 address            0x48
sensitivity                10.0 µV/(W/m²)
offset                     0.0 µV
maximum                    2000 W/m²
clamp negative             true
```

Dati esposti:

- ADC raw;
- millivolt differenziali;
- radiazione in W/m²;
- sensibilità configurata;
- indirizzo ADS1115 rilevato;
- contatore errori;
- ultimo errore;
- stato `enabled` / `ok`.

### Cablaggio e polarità

Il segnale del piranometro deve arrivare all'ADS1115 come coppia differenziale:

```text
RSG1-N OUT+  → ADS1115 A0
RSG1-N OUT-  → ADS1115 A1
```

Se la polarità viene invertita, il segnale risulta negativo; con `clampNegative = true` il firmware lo porterebbe a zero, mascherando il cablaggio errato. In fase di collaudo è quindi opportuno verificare anche la tensione differenziale grezza in mV.

### Installazione metrologica

Per ottenere una misura significativa il piranometro deve essere:

- montato perfettamente in piano;
- libero da ombre permanenti o intermittenti;
- lontano per quanto possibile da superfici riflettenti anomale;
- mantenuto pulito;
- installato con cablaggio schermato/ordinato, evitando accoppiamenti con linee di potenza e sorgenti RF;
- configurato con la costante del proprio certificato di taratura.

La cupola e la superficie ottica devono essere ispezionate periodicamente: sporco, condensa o depositi producono errori che il firmware non può correggere.

### Fail-safe

Il firmware verifica la presenza dell'ADS1115 prima della lettura. In caso di disconnessione:

1. il sensore passa a `ok=false`;
2. l'indirizzo rilevato torna non valido;
3. viene incrementato il contatore errori;
4. Web/MQTT e gli altri sensori continuano;
5. l'inizializzazione viene ritentata al ciclo successivo.

---

# 3. BME280

Sensore digitale Bosch per temperatura, umidità relativa e pressione barometrica.

### Caratteristiche principali

| Parametro | Valore indicativo da datasheet Bosch |
|---|---|
| Pressione | `300 … 1100 hPa` |
| Temperatura operativa | `-40 … +85 °C` |
| Umidità | `0 … 100 %RH` |
| Accuratezza umidità | circa `±3 %RH` |
| Tempo di risposta umidità | circa `1 s` |
| Interfacce native | I2C / SPI |

### Nel progetto

```text
I2C default     0x76
fallback        0x77
SDA             GPIO21
SCL             GPIO22
```

Sono configurabili offset per temperatura, pressione e umidità. Il firmware calcola anche il dew point.

Il BME280 è molto utile come sensore ambientale generale e come confronto operativo con il TA-N, ma la temperatura del chip può essere influenzata dall'installazione elettronica e dal calore interno del contenitore.

---

# 4. DHT11

Sensore digitale economico di temperatura e umidità.

### Caratteristiche tipiche

La documentazione Aosong più recente indica, a seconda della revisione, un campo esteso fino a circa `-20 … +60 °C` e `5 … 95 %RH`; le specifiche storiche più diffuse riportano `0 … 50 °C` e `20 … 90 %RH`.

Accuratezza tipica comunemente dichiarata:

```text
temperatura   ±2 °C
umidità       ±5 %RH
```

### Nel progetto

```text
DATA           GPIO14
```

Sono disponibili offset di temperatura e umidità.

Il DHT11 è da considerare un sensore **secondario / di controllo**, non uno strumento metrologico equivalente al NESA TA-N.

---

# 5. BH1750

Sensore digitale di illuminamento ambientale con interfaccia I2C.

### Nel progetto

```text
indirizzo default   0x23
fallback            0x5C
SDA                 GPIO21
SCL                 GPIO22
```

Il dato viene espresso in **lux**. È disponibile un offset configurabile.

Il BH1750 misura l'illuminamento percepito secondo una risposta ottica orientata alla luce visibile; non è un piranometro e quindi **non può sostituire il NESA RSG1-N** per la misura energetica della radiazione solare in W/m².

---

# 6. INA219

Monitor digitale Texas Instruments per tensione, corrente e potenza tramite shunt.

### Caratteristiche principali

| Parametro | Valore |
|---|---|
| Tensione bus | `0 … 26 V` |
| Conversione | 12 bit |
| Interfaccia | I2C / SMBus |
| Indirizzi | 16 configurazioni possibili |
| Temperatura componente | `-40 … +125 °C` |

### Nel progetto

```text
I2C default          0x40
calibrazione default 32 V / 2 A
```

Vengono pubblicati:

- tensione bus;
- tensione shunt;
- tensione carico;
- corrente;
- potenza.

Il valore di corrente dipende dalla resistenza shunt e dalla calibrazione della scheda effettivamente utilizzata.

---

# 7. Sensore UV analogico / GUVA

Il progetto tratta l'ingresso UV come **sensore analogico lineare configurabile** su ADC1 dell'ESP32.

### Nel progetto

```text
ADC                  GPIO34
modalità default      GUVA 100 mV/UVI
zero                  0 mV
scala                 100 mV/UVI
campioni              32
UVI massimo           20
```

Il pin è vincolato ad ADC1 (`GPIO32…39`) per evitare i conflitti tipici di ADC2 durante l'uso Wi-Fi.

> La modalità `100 mV/UVI` è una **calibrazione firmware**, non una dichiarazione universale valida per qualsiasi modulo UV. Per una misura quantitativa affidabile occorre utilizzare la curva/calibrazione del sensore realmente montato.

---

# 8. SDS011

Sensore laser Nova Fitness per particolato atmosferico.

### Caratteristiche principali

| Parametro | Valore indicativo |
|---|---|
| Grandezze | PM2.5, PM10 |
| Campo | circa `0 … 999,9 µg/m³` |
| Alimentazione | 5 V |
| Corrente attiva | circa 70 mA ±10 mA nelle revisioni classiche |
| Corrente sleep | `< 4 mA` |
| Output seriale | 1 Hz |
| Temperatura operativa | circa `-10 … +50 °C` |
| Umidità operativa | fino a circa 70 %RH secondo datasheet classico |

### Nel progetto

```text
ESP32 RX        GPIO16 ← SDS TX
ESP32 TX        GPIO17 → SDS RX
UART            9600 8N1
```

Politica di misura default:

```text
ciclo           60 min
warm-up         30 s
campioni        5
max awake       120 s
```

Solo l'SDS011 viene messo in sleep; l'ESP32 resta sempre acceso.

L'umidità elevata può influenzare in modo importante i sensori ottici di particolato: per uso quantitativo è opportuno confrontare i dati con una stazione di riferimento e considerare eventuali correzioni locali.

---

# 9. AS3935

Circuito dedicato al rilevamento dell'attività elettrica atmosferica.

Il firmware distingue gli eventi:

```text
noise
disturber
lightning
```

ed espone distanza stimata, energia e contatori evento quando disponibili.

### Nel progetto

```text
I2C default      0x03
fallback         0x02, 0x01, 0x00
IRQ              GPIO27
profilo           outdoor
```

Parametri configurabili:

- noise floor;
- watchdog threshold;
- spike rejection;
- lightning threshold;
- mask disturber.

La qualità della rilevazione dipende fortemente dalla taratura dell'antenna, dalla disposizione fisica e dal rumore elettromagnetico generato da alimentatori switching, display, convertitori DC/DC e cablaggi digitali.

---

# 10. Gerarchia consigliata delle misure

Per l'uso meteorologico del nodo si suggerisce la seguente priorità concettuale:

| Grandezza | Sensore primario | Sensore secondario / diagnostico |
|---|---|---|
| Temperatura aria | **NESA TA-N** | BME280 / DHT11 |
| Radiazione solare globale | **NESA RSG1-N** | BH1750 solo come confronto di luce, non equivalente |
| Pressione | BME280 | — |
| Umidità relativa | BME280 | DHT11 |
| PM2.5 / PM10 | SDS011 | — |
| UV Index | UV analogico calibrato | — |
| Fulmini | AS3935 | — |
| Alimentazione | INA219 | — |

Il firmware può continuare a pubblicare tutti i dati, ma le grandezze NESA devono essere considerate quelle di riferimento quando i relativi sensori sono installati, correttamente tarati e `ok=true`.

---

# 11. Collaudo consigliato dei NESA

## TA-N

1. verificare sul breakout MAX31865 che la RREF sia quella prevista per PT100, circa 430 Ω;
2. verificare i ponticelli/jumper per modalità 4 fili;
3. controllare continuità dei quattro conduttori della PT100;
4. verificare che a temperatura ambiente la resistenza letta sia coerente con una PT100;
5. confrontare la temperatura con un riferimento noto e applicare un offset solo se giustificato;
6. simulare scollegamento di un filo e verificare che il firmware segnali fault senza reboot.

## RSG1-N

1. leggere la sensibilità dal certificato del sensore;
2. inserirla nella configurazione Web;
3. verificare `OUT+ → A0` e `OUT- → A1`;
4. a sensore oscurato verificare offset prossimo allo zero atteso;
5. in pieno sole controllare che il segnale sia positivo e dell'ordine di pochi/massimo alcune decine di mV;
6. confrontare `millivolts` e `radiation_wm2` con la formula di conversione;
7. scollegare ADS1115 e verificare il passaggio a errore senza reboot;
8. ricollegare ADS1115 e verificare il recupero automatico.

---

# 12. Manutenzione

Per mantenere dati coerenti nel tempo:

- controllare periodicamente morsetti e ossidazione;
- evitare giunzioni non protette all'esterno;
- mantenere pulita la schermatura del TA-N;
- mantenere pulite cupola/superficie del RSG1-N;
- controllare il livellamento del piranometro;
- conservare il certificato di taratura RSG1-N e la relativa costante;
- annotare ogni modifica di RREF, breakout MAX31865, ADS1115 o cablaggio;
- dopo interventi hardware verificare la pagina **Diagnostica → Mappa pin** e i valori raw.

---

# 13. Fonti tecniche

Fonti principali consultate per questo documento:

- NESA — TA/TAV, temperatura aria PT100 1/3 DIN, 4 fili: `https://www.nesasrl.eu/wp-content/uploads/2022/12/1.TATAV_IT.pdf`
- NESA — RSG1, piranometro a termopila: `https://www.nesasrl.eu/wp-content/uploads/2022/12/2.RSG1_IT.pdf`
- Analog Devices — MAX31865 RTD-to-Digital Converter: `https://www.analog.com/en/products/max31865.html`
- Texas Instruments — ADS1115 16-bit ADC: `https://www.ti.com/product/ADS1115`
- Bosch Sensortec — BME280: `https://www.bosch-sensortec.com/en/products/environmental-sensors/humidity-sensors-bme280/`
- Texas Instruments — INA219: `https://www.ti.com/product/INA219`
- Aosong — DHT11 datasheet/revision: documentazione tecnica del costruttore
- Nova Fitness — SDS011: datasheet e protocollo seriale del produttore

Per i valori elettrici massimi, le tolleranze e le condizioni limite fare sempre riferimento alla revisione del datasheet corrispondente all'hardware realmente installato.

---

# 14. Riferimenti nel repository

- [`NESA.md`](NESA.md) — integrazione software dei due sensori NESA;
- [`PINOUT.md`](PINOUT.md) — pin e bus del progetto;
- [`CONFIGURATION.md`](CONFIGURATION.md) — parametri configurabili;
- [`MQTT.md`](MQTT.md) — payload e telemetria;
- [`ROBUSTNESS.md`](ROBUSTNESS.md) — fail-safe e gestione errori.
