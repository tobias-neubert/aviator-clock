# 01 Fix ESP32 Build Baseline and Replace Deprecated ESP32Encoder

Status: ready-for-agent
Blocked by: none

## Context
When running `pio run -e nodemcu-32s`, the build fails because PlatformIO pulls `espressif32@8.0.0` (Arduino-ESP32 Core 3.x / ESP-IDF 5.x), whereas the third-party library `madhephaestus/ESP32Encoder @ ^0.10.2` relies on the removed ESP-IDF 4.x legacy PCNT driver and obsolete FreeRTOS spinlock macros (`portMUX_TYPE`). Furthermore, `platformio.ini` has duplicate debug level definitions triggering compiler warnings.

## Requirements
1. Pin the ESP32 platform in `platformio.ini` to a stable tested version (or replace `ESP32Encoder` with a lightweight, interrupt-driven quadrature decoder in `src/platform/input/RotaryEncoder.h`).
2. Clean up compiler flags in `platformio.ini` to prevent `"CORE_DEBUG_LEVEL" redefined` warnings.
3. Verify that `pio run -e nodemcu-32s` succeeds.
4. Verify that `pio test -e google_tests` continues to pass.

## Verification
- Run `pio run -e nodemcu-32s` and ensure exit code 0.
- Run `pio test -e google_tests` and ensure all 38 tests pass.

## Comments
