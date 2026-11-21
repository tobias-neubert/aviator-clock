# 01 Fix ESP32 Build Baseline, Debug Flags & Clean Obsolete Encoders

Status: closed
Blocked by: none

## Context
When running `pio run -e nodemcu-32s`, the build had duplicate debug level definitions triggering compiler warnings (`CORE_DEBUG_LEVEL redefined`), and the codebase previously contained deprecated `madhephaestus/ESP32Encoder` references. With the simplification for Version 1.0, physical rotary encoders and DIP switches are retired in favor of web-based configuration.

## Requirements
1. Remove deprecated `madhephaestus/ESP32Encoder` dependency and obsolete encoder drivers.
2. Clean up compiler flags in `platformio.ini` (`build_unflags = -DCORE_DEBUG_LEVEL=0`) to eliminate `"CORE_DEBUG_LEVEL" redefined` warnings.
3. Verify that `pio run -e nodemcu-32s` builds cleanly with zero compiler warnings.
4. Verify that `pio test -e google_tests` passes all tests.

## Verification
- `pio run -e nodemcu-32s` succeeds with exit code 0 and zero warnings.
- `pio test -e google_tests` succeeds with exit code 0.

## Comments
- Completed during baseline simplification for Version 1.0. Both tests and firmware compilation succeed cleanly.
