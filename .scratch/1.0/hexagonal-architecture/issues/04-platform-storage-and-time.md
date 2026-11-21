# 04 Implement Platform Storage (NVS) and Time (SNTP) Adapters

Status: closed
Blocked by: 02

## Context
Currently, persistent storage and time are split across `soc-api` (`IPersistentStorage`, `ITime`) and `soc-esp32` (`ESP32PersistentStorage`, `NtpTime`). We must implement them as direct, clean adapters of the domain ports `AlarmStorage` and `TimeSource`.

## Requirements
1. Create `src/platform/storage/NvsAlarmStorage.h/.cpp` in namespace `aviator::platform::storage`:
   - Implements `aviator::domain::AlarmStorage`.
   - Uses ESP32 `Preferences` directly to persist `Alarm` configuration (hour, minute, enabled, sound track ID).
   - Eliminates intermediate layers (`IPersistentStorage`, `AlarmRepository`).
2. Create `src/platform/time/NtpTimeSource.h/.cpp` in namespace `aviator::platform::time`:
   - Implements `aviator::domain::TimeSource`.
   - Encapsulates ESP-IDF SNTP initialization and POSIX `time()` querying.
3. Add native mock implementations for `AlarmStorage` and `TimeSource` in `test/mocks/` for GoogleTest.

## Verification
- Run `pio test -e google_tests` to verify alarm persistence and time component translation.
- Verify compilation with `pio run -e nodemcu-32s`.

## Comments
- Implemented `NvsAlarmStorage` in `src/platform/storage/` implementing domain port `aviator::domain::AlarmStorage` directly using ESP32 `Preferences` (NVS) for persisting hour, minute, enabled, and sound track ID.
- Implemented `NtpTimeSource` in `src/platform/time/` implementing domain port `aviator::domain::TimeSource` encapsulating ESP-IDF SNTP daemon initialization, POSIX timezone handling (`setenv`/`tzset`), and wall-clock component conversion (`localtime_r`).
- Added native `Preferences` mock in `test/mocks/Preferences.h` and test doubles `FakeAlarmStorage.h` and `FakeTimeSource.h` in `test/mocks/`.
- Created comprehensive GoogleTest suites in `test/gtests/test_storage/` (9 unit tests) and `test/gtests/test_time/` (11 unit tests).
- Verified 100% test pass (95/95 test cases) with `pio test -e google_tests` and clean firmware build with `pio run -e nodemcu-32s`.
