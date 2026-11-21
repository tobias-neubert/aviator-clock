# 04 Implement Platform Storage (NVS) and Time (SNTP) Adapters

Status: ready-for-agent
Blocked by: 02

## Context
Currently, persistent storage and time are split across `soc-api` (`IPersistentStorage`, `ITime`) and `soc-esp32` (`ESP32PersistentStorage`, `NtpTime`). We must implement them as direct, clean adapters of the domain ports `AlarmStorage` and `TimeSource`.

## Requirements
1. Create `src/platform/storage/NvsAlarmStorage.h/.cpp` in namespace `aviator::platform::storage`:
   - Implements `aviator::domain::AlarmStorage`.
   - Uses ESP32 `Preferences` directly to read and write alarm times for slots 0–7.
   - Eliminates unnecessary intermediate layers (`IPersistentStorage`, `AlarmRepository`).
2. Create `src/platform/time/NtpTimeSource.h/.cpp` in namespace `aviator::platform::time`:
   - Implements `aviator::domain::TimeSource`.
   - Encapsulates ESP-IDF SNTP initialization and POSIX `time()` querying.
3. Add native mock implementations for `AlarmStorage` and `TimeSource` in `test/mocks/` for GoogleTest.

## Verification
- Run `pio test -e google_tests` to verify alarm persistence and time component translation.
- Verify compilation with `pio run -e nodemcu-32s`.

## Comments
