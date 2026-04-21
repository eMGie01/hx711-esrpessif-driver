# README — HX711 ESP-IDF Driver (minimal)

A minimal HX711 driver for ESP-IDF focused on reliable raw ADC readout with optional interrupt-assisted 'data ready' signaling.  
The driver intentionally avoids application-level concepts such as scaling, units, tare, filtering, or averaging.

## Key characteristics

- Returns **raw signed 24-bit ADC samples** (sign-extended to `int32_t`)
- Supports:
  - **Polling read** (`hx711_read_raw`)
  - **Timeout-based read** without task/tick dependencies (`hx711_read_raw_with_timeout`)
  - **ISR-assisted read** (`hx711_init_with_isr` + `hx711_read_raw_isr`)
- Bit-banged readout is protected by a **critical section** to avoid corrupted reads due to interrupts/context switches.
- Uses **C11 atomics** for safe ISR → main code synchronization.
- Uses `esp_timer_get_time()` for timeout deadlines (microseconds resolution).

## Files

- `src/hx711.h` – public API and types  
- `src/hx711_init.c` – initialization/deinitialization, ISR setup  
- `src/hx711_meas.c` – ready check and raw readout functions  

## Requirements

- ESP-IDF with GPIO driver support (`driver/gpio.h`)
- `esp_timer` available (for timeout reads)
- FreeRTOS task API available (`vTaskDelay` is used by the ISR-assisted wait path)
- C11 atomics (`<stdatomic.h>`)

Notes:

- `hx711_read_raw_with_timeout()` remains independent from FreeRTOS task scheduling.
- `hx711_read_raw_isr()` is intended for **task context only** and uses `vTaskDelay()` while waiting for the ISR readiness flag.
- A small dependency on FreeRTOS remains for `portMUX_TYPE` / critical sections and the task-delay based wait path.

## Public API (Overview)

### Initialization

- `hx711_init(hx711_t *dev, hx711_hw_t *gpios, const hx711_set_t *settings)`
- `hx711_init_default(hx711_t *dev, hx711_hw_t *gpios)`
- `hx711_init_with_isr(hx711_t *dev, hx711_hw_t *gpios, const hx711_set_t *settings)`
- `hx711_deinit(hx711_t *dev)`

### Measurement

- `hx711_is_ready(const hx711_t *dev)`
- `hx711_read_raw(hx711_t *dev, int32_t *value)`
- `hx711_read_raw_with_timeout(hx711_t *dev, int32_t *value)`
- `hx711_read_raw_isr(hx711_t *dev, int32_t *value)`

## Configuration

### GPIO mapping

Use `hx711_hw_t` to provide pin numbers:

- `io_dout` – HX711 DOUT (data ready + data line), configured as input  
- `io_sck` – HX711 SCK (clock), configured as output  

### Mode / gain selection

The HX711 gain/channel selection is controlled by the number of additional SCK pulses after the 24-bit read.  
The driver exposes it via `hx711_mode_t` in `hx711_set_t`.

### Timeout

`timeout_ms` is used by:

- `hx711_read_raw_with_timeout()`
- `hx711_read_raw_isr()`

Timeout measurement is done using `esp_timer_get_time()` (microseconds - `us`).

## Typical usage patterns

### 1. Polling (non-blocking)

1. Call `hx711_is_ready()`.
2. If ready, call `hx711_read_raw()`.

Use this when you already have a main loop and want full control over scheduling.

### 2. Timeout-based blocking read (no FreeRTOS task dependency)

Call `hx711_read_raw_with_timeout()` and let the driver wait until:

- DOUT goes low (data ready), then read, or
- timeout expires.

### 3. ISR-assisted read

Use `hx711_init_with_isr()` to configure a negative-edge interrupt on `io_dout`.  
Then call `hx711_read_raw_isr()`, which waits for the ISR-updated readiness flag, yields the calling task while waiting, and performs a protected read once data is ready.

This mode is useful when you want to avoid continuous busy-polling of `DOUT`, but it must be called from a FreeRTOS task rather than from an ISR.

## Design notes / scope

This driver is intentionally **low-level**:

- It provides raw readings only.
- Calibration (offset/scale), tare, filtering, moving averages, unit conversions, and “weight” logic should be implemented in the application layer.

## Error handling

All functions return `hx711_status_t`. Typical error reasons:

- invalid arguments (NULL pointers, invalid mode)
- not initialized
- GPIO configuration failures
- timeout while waiting for data ready
- ISR setup failures (when using ISR-assisted mode)
