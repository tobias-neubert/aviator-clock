# 08 Remediation: Zero Dynamic Allocation at Runtime and Concurrency Safety

Status: ready-for-agent
Blocked by: 07

## Context
A code review of the current hexagonal architecture implementation against the repository's **Embedded C++ & FreeRTOS Engineering Standards** (`AGENTS.md` and `docs/ARCHITECTURE.md`) identified several runtime heap allocations and concurrency gaps:
1. `WebPortalServer` performs dynamic heap allocations in HTTP callbacks (`std::map<String, int>`, Arduino `String`, ArduinoJson 7 `JsonDocument`).
2. FreeRTOS tasks (`deferred_restart_task`, `deferred_stop_task`) are dynamically allocated on the heap via `xTaskCreate` during REST requests.
3. `AsyncWebServer` is dynamically allocated with `std::make_unique` and destroyed via `.reset()` on each mode change.
4. Shared mutable state across FreeRTOS tasks (`AlarmCoordinator`, `OperatingStateMachine`, and `Jq6500AudioPlayer`) lacks thread synchronization (mutexes / atomics).

## Requirements

### 1. Zero Runtime Dynamic Allocation in `WebPortalServer`
- **Replace `std::map<String, int>` in `/api/wifi/scan`:** Use a fixed-size array (`std::array<ScannedNetwork, 16>`) with in-place deduplication and RSSI sorting.
- **Eliminate Arduino `String` response buffers:** Stream responses directly with `request->beginResponseStream("application/json")` or use fixed-size stack buffers (`snprintf`).
- **Eliminate runtime `xTaskCreate` / `vTaskDelete`:**
  - Remove on-the-fly task creation in `/api/system/reboot` and `/api/setup/finish`.
  - Use non-allocating flags (e.g., `std::atomic<bool> _pendingReboot`, `_pendingSetupFinish`) inspected by the main clock loop or static FreeRTOS software timers (`xTimerCreateStatic`).
- **Persistent Server Instance:** Ensure `AsyncWebServer` is pre-instantiated or maintained as a persistent member object without `make_unique`/`reset` allocations during mode switches.

### 2. Concurrency Safety Across Tasks
- **`AlarmCoordinator`:**
  - Convert flags (`_isRinging`, `_isDismissedForCurrentAlarmEvent`) to `std::atomic<bool>`.
  - Protect `_alarm` read/write access across tasks (`clock_engine_task`, `setup_btn_task`, and `AsyncWebServer`) with a mutex or thread-safe copy mechanism.
- **`OperatingStateMachine`:**
  - Make `_currentMode` an `std::atomic<OperatingMode>` to ensure lock-free, atomic reads from `clock_engine_task` and the web server task.
- **`Jq6500AudioPlayer`:**
  - Serialize UART transmissions in `sendCommand()` with a mutex to avoid corrupting frames when alarm audio and web preview trigger concurrently.

## Verification
- Run `pio test -e google_tests` to verify 100% pass across all unit test suites.
- Run `pio run -e nodemcu-32s` to confirm zero compiler errors/warnings on the target hardware platform.
- Verify through code inspection that no `new`, `malloc`, `std::map`, or `String` allocations occur during running task loops and HTTP request callbacks.

## Comments
- Completed thread-safety hardening for `AlarmCoordinator` and `OperatingStateMachine` (Requirement 2):
  - `OperatingStateMachine::_currentMode` converted to `std::atomic<OperatingMode>` with CAS transitions.
  - `AlarmCoordinator` synchronized using a lock-free seqlock (`_alarmSequence`), atomic writer spinlock (`_writerLock`), and atomic booleans (`_isRinging`, `_isDismissedForCurrentAlarmEvent`), returning `Alarm` by value snapshot.
  - Added multi-threaded concurrency unit tests in `test_AlarmCoordinator.cpp` and `test_OperatingStateMachine.cpp` (all 108 tests passing).
- Completed zero runtime dynamic allocation in `WebPortalServer` and UART synchronization in `Jq6500AudioPlayer` (Requirement 1 and remainder of Requirement 2):
  - Eliminated `std::map<String, int>` in `/api/wifi/scan`, replacing with fixed `std::array<ScannedNetworkEntry, 16>` and in-place deduplication.
  - Replaced intermediate `String response` buffers with direct `AsyncResponseStream` streaming and static JSON for `/api/audio/tracks`.
  - Replaced runtime `xTaskCreate` / `vTaskDelete` in `/api/system/reboot` and `/api/setup/finish` with atomic timestamp triggers evaluated non-blockingly in `WebPortalServer::update()`.
  - Converted `AsyncWebServer` to persistent member initialized once during boot.
  - Guarded `Jq6500AudioPlayer::sendCommand()` with `std::mutex` and added concurrency unit test (all 109 tests passing).

