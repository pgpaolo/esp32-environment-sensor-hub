# Verifica generale v0.7.3

Controllo funzionale/documentale del repository `esp32-environment-sensor-hub` dopo l'introduzione delle modifiche di robustezza.

## Stato verificato

| Area | Verifica | Stato |
|---|---|---|
| Versione firmware | `FW_VERSION = 0.7.3` | OK |
| Build PlatformIO | `esp32dev` | OK |
| RAM statica | 51.036 / 327.680 byte (15,6%) | OK |
| Flash applicativa | 1.134.213 / 1.966.080 byte (57,7%) | OK |
| Web statico | `src/WebAssets.h` in PROGMEM | OK |
| JSON Web | serializzazione diretta su WiFiClient | OK |
| Heap diagnostics | free/min/largest/fragmentation | OK |
| Config schema | `cfgver = 1` | OK |
| Migrazione NVS | load + default + validate + save | OK |
| Password Web | non restituita da `/api/config` | OK |
| Password Wi-Fi | non restituita da `/api/config` | OK |
| Password MQTT | non restituita da `/api/config` | OK |
| CA MQTT | testo non restituito da `/api/config` | OK |
| MQTT LWT | retained `online/offline` | OK |
| MQTT backoff | progressivo fino a 60 s | OK |
| MQTT statistiche | connect/disconnect/publish separate | OK |
| Sensori disabilitabili | singolarmente da configurazione | OK |
| Pulsante PIN | card sensori + mappa completa | OK |
| AP manutenzione | `192.168.4.1` | OK |
| Web auth default | `admin/admin` | OK |
| NESA TA-N | MAX31865 / PT100 4 fili | OK |
| NESA RSG1-N | ADS1115 differenziale | OK |
| RF Oregon | assente dal progetto | OK |

## NESA TA-N

Baseline verificata nel codice:

```text
MAX31865
MAX31865_4WIRE
R0   = 100 ohm
RREF = 430 ohm
CS   = GPIO13
```

Il firmware è quindi predisposto per il TA-N/PT100 a 4 fili tramite MAX31865. MAX31855 non è compatibile con questa configurazione.

## NESA RSG1-N

Baseline:

```text
ADS1115
I2C 0x48
A0-A1 differenziale
GAIN_SIXTEEN
128 SPS
```

La sensibilità deve essere impostata secondo il certificato del piranometro.

## Configurazione

La configurazione principale è salvata in `sensorhub`; NESA in `sensorhub_nesa`.

La validazione copre GPIO, ADC1, bus I2C, indirizzi, intervalli generali, SDS011, AS3935 e parametri NESA.

## Web UI

Le pagine principali non richiedono più grandi buffer `String` dinamici. Dashboard/configurazione/OTA sono servite da asset statici in flash.

Le API dinamiche sono documentate in `docs/API.md`.

## MQTT

Il buffer PubSubClient è 4096 byte. Il payload corrente include anche i due NESA e le statistiche MQTT.

Il backoff si resetta al valore base dopo una connessione riuscita.

## Note di test hardware

La CI verifica compilazione e dimensioni, ma non sostituisce il collaudo sul dispositivo reale. Dopo il flash verificare:

1. avvio e reset reason;
2. heap/min heap/largest block dopo più accessi Web;
3. scansione I2C;
4. stato MQTT e LWT;
5. ciclo SDS011 sleep/wake;
6. fault handling MAX31865;
7. lettura ADS1115 con sensibilità reale;
8. pin map e sensori disabilitati;
9. salvataggio configurazione e riavvio;
10. factory reset e accesso `admin/admin`.
