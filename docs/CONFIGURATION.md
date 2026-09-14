# Riferimento configurazione

Questo documento descrive la configurazione di ESP32 Environment Sensor Hub **v0.7.3**.

La configurazione viene salvata in NVS e applicata al riavvio dopo **Salva e riavvia**.

## Accesso

Web UI:

```text
http://<ip-esp32>/
```

AP di manutenzione:

```text
http://192.168.4.1/
```

Credenziali factory:

```text
user: admin
password: admin
```

Le password salvate non vengono precompilate nei campi Web. Un campo password lasciato vuoto mantiene il valore precedente.

## NVS e versioning

Namespace:

```text
sensorhub
sensorhub_nesa
```

Schema logico corrente:

```text
cfgver = 1
```

La chiave `cfgver` è memorizzata nel namespace principale `sensorhub`. `sensorhub_nesa` contiene i parametri NESA, ma viene caricato nello stesso oggetto `AppConfig` e sottoposto alla validazione finale insieme al resto della configurazione.

Sequenza di avvio:

```text
load sensorhub → validate/migrate → load sensorhub_nesa → validate completo → save correzioni
```

Le chiavi mancanti usano i default firmware. Valori non validi vengono corretti singolarmente dalla validazione.

## Rete Wi-Fi

Default principali:

| Parametro | Default |
|---|---|
| Nome dispositivo | `esp32-sensor` |
| IPv4 statico | disabilitato |
| IP proposto | `192.168.1.221` |
| Gateway | `192.168.1.1` |
| Subnet | `255.255.255.0` |
| DNS1 | `192.168.1.1` |
| DNS2 | `1.1.1.1` |
| Timezone | `CET-1CEST,M3.5.0,M10.5.0/3` |

Se la STA non si collega, il firmware avvia l'AP di manutenzione `192.168.4.1`.

Il pulsante BOOT/config mantenuto premuto all'avvio forza la modalità di manutenzione.

## Web / sicurezza

| Parametro | Default |
|---|---|
| Utente Web | `admin` |
| Password Web | `admin` |

La password Web non viene restituita da `/api/config`.

Opzioni:

- nuovo valore nel campo password → sostituisce la password;
- campo vuoto → mantiene quella salvata;
- ripristino Web → torna a `admin/admin`.

La cifratura NVS non è abilitata per scelta progettuale.

## MQTT

| Parametro | Default / limite |
|---|---|
| Porta | `1883` |
| Base topic | `sensors/esp32-sensor` |
| Retain telemetria | disabilitato |
| TLS | disabilitato |
| TLS insecure | abilitato quando TLS è usato |
| Reconnect base | `5 s`, valido `1..300 s` |

Il backoff automatico cresce fino a 60 s dopo errori consecutivi.

Password e CA salvate non vengono ritrasmesse al browser.

## Intervalli generali

| Parametro | Default | Range validato |
|---|---:|---:|
| Campionamento sensori lenti | 30 s | 2..86400 s |
| Telemetria MQTT | 60 s | 5..86400 s |

## Bus I2C

| Parametro | Default |
|---|---:|
| SDA | GPIO21 |
| SCL | GPIO22 |

SDA e SCL non possono coincidere. GPIO6..11 sono esclusi perché normalmente riservati alla flash ESP32.

## Sensori attivi

Default:

| Sensore | Stato |
|---|---|
| BH1750 | ON |
| BME280 | ON |
| DHT11 | ON |
| UV analogico | ON |
| INA219 | ON |
| SDS011 | ON |
| AS3935 | ON |
| NESA TA-N | OFF |
| NESA RSG1-N | OFF |

Un sensore OFF non viene inizializzato né campionato e non concorre allo stato Health.

## BH1750

| Parametro | Default |
|---|---:|
| Indirizzo | `0x23` |
| Offset | `0 lux` |

Indirizzi accettati: `0x23`, `0x5C`.

## BME280

| Parametro | Default |
|---|---:|
| Indirizzo | `0x76` |
| Offset temperatura | `0 °C` |
| Offset pressione | `0 hPa` |
| Offset umidità | `0 %` |

Indirizzi accettati: `0x76`, `0x77`.

## DHT11

| Parametro | Default |
|---|---:|
| DATA | GPIO14 |
| Offset temperatura | `0 °C` |
| Offset umidità | `0 %` |

Il DHT11 richiede un GPIO bidirezionale/output-capable; GPIO34..39 non sono validi per questa funzione.

## INA219

| Parametro | Default |
|---|---:|
| Indirizzo | `0x40` |
| Calibrazione | 32 V / 2 A |
| Offset bus | `0 V` |
| Offset corrente | `0 mA` |

Range indirizzi validato: `0x40..0x4F`.

## UV analogico

| Parametro | Default |
|---|---:|
| GPIO | 34 |
| Modalità | GUVA 100 mV/UVI |
| Zero | `0 mV` |
| Scala | `100 mV/UVI` |
| Campioni | 32 |
| Gap | 1500 µs |
| UVI massimo | 20 |

Il pin UV deve appartenere ad ADC1 `GPIO32..39`.

## SDS011

| Parametro | Default | Range validato |
|---|---:|---:|
| ESP32 RX | GPIO16 | GPIO valido |
| ESP32 TX | GPIO17 | GPIO output valido |
| Ciclo | 60 min | 1..1440 min |
| Warm-up | 30 s | 15..180 s |
| Campioni | 5 | 1..30 |
| Gap campioni | 1000 ms | 250..10000 ms |
| Max awake | 120 s | 30..600 s |
| Primo ciclo | 10 s | 0..600 s |

RX e TX non possono coincidere.

L'ESP32 resta sempre acceso; solo SDS011 viene posto in sleep.

## AS3935

| Parametro | Default |
|---|---:|
| Indirizzo | `0x03` |
| IRQ | GPIO27 |
| Profilo | outdoor |
| Noise floor | 2 |
| Watchdog | 2 |
| Spike rejection | 2 |
| Lightning threshold | 1 |
| Mask disturber | OFF |

Indirizzi validi: `0x00..0x03`.

Threshold ammessi dal firmware: `1`, `5`, `9`, `16`.

## NESA TA-N / MAX31865

Default:

| Parametro | Default |
|---|---:|
| Abilitato | OFF |
| CS | GPIO13 |
| RTD nominale | 100 ohm |
| RREF | 430 ohm |
| Offset temperatura | 0 °C |

Baseline prevista: **PT100 4 fili**.

Il sensore richiede il **MAX31865**. Il MAX31855 non è compatibile perché è destinato alle termocoppie. Breakout Adafruit-compatible/DollaTek equivalenti vanno bene se configurati per PT100 e RREF circa 430 ohm.

## NESA RSG1-N / ADS1115

Default:

| Parametro | Default |
|---|---:|
| Abilitato | OFF |
| Indirizzo ADS1115 | `0x48` |
| Sensibilità | `10 µV/(W/m²)` |
| Offset | `0 µV` |
| Max software | `2000 W/m²` |
| Clamp negativo | ON |

Indirizzi accettati: `0x48..0x4B`.

La sensibilità reale va sostituita con quella del certificato di taratura del sensore.

## Relay e LED

| Funzione | Default |
|---|---:|
| Relay | GPIO15 |
| Relay abilitato | ON |
| Relay invertito | OFF |
| LED stato | GPIO12 |
| LED abilitato | ON |
| LED invertito | OFF |

GPIO12 e GPIO15 sono pin di strapping: mantenere un cablaggio compatibile con il boot ESP32.

## Factory reset

Il factory reset cancella entrambi i namespace NVS. Al riavvio tornano i default firmware, incluse le credenziali Web `admin/admin`.

## Validazione automatica

La validazione protegge almeno da:

- porta MQTT 0;
- reconnect/intervalli fuori range;
- SDA = SCL;
- RX SDS = TX SDS;
- GPIO6..11;
- GPIO34..39 usati come output/pull-up;
- UV fuori ADC1;
- indirizzi I2C fuori range noto;
- collisione INA219/ADS1115 sullo stesso indirizzo quando entrambi attivi;
- parametri SDS011/AS3935/NESA fuori range;
- credenziali Web vuote o eccessivamente lunghe.

Quando un parametro non è valido viene ripristinato il suo default senza cancellare il resto della configurazione.
