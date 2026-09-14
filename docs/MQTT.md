# MQTT schema

Base topic predefinito: `sensors/esp32-sensor`

## Availability

`<base>/status`

Payload retained: `online` / `offline`.

## Telemetry

`<base>/telemetry`

La telemetria JSON include stato sistema, relay, BH1750, BME280, DHT11, INA219, UV, SDS011 e AS3935. Il campo `reason` indica la causa della pubblicazione (`periodic` oppure `sensor_event`).

Esempio ridotto:

```json
{
  "device": "esp32-sensor",
  "reason": "periodic",
  "bme280": {
    "ok": true,
    "temperature_c": 22.4,
    "humidity_pct": 61.1,
    "pressure_hpa": 954.4
  },
  "uv": {
    "ok": true,
    "raw_adc": 512,
    "millivolts": 425,
    "uv_index": 4.25
  },
  "sds011": {
    "ok": true,
    "state": "sleeping",
    "pm25_ugm3": 3.4,
    "pm10_ugm3": 8.1,
    "next_measurement_s": 3520
  }
}
```
