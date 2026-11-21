# 07 Assemble CompositionRoot, Migrate main.cpp, and Remove Legacy lib/

Status: closed
Blocked by: 03, 04, 05, 06

## Context
With all domain components, ports, and platform adapters implemented, the final step is to assemble the dependency injection wiring in `CompositionRoot`, rewire `src/main.cpp`, delete the legacy libraries in `lib/`, and clean up `platformio.ini`.

## Requirements
1. Implement `src/app/CompositionRoot.h/.cpp` in namespace `aviator::app`:
   - Instantiates platform adapters (`StepperSubsystem`, `NvsAlarmStorage`, `NtpTimeSource`, `SetupButtonAdapter`, `Jq6500AudioPlayer`, `WebPortalServer`).
   - Instantiates domain services and state machine (`ClockEngine`, `AlarmCoordinator`, `OperatingStateMachine`).
   - Wires platform adapters to domain ports via dependency injection.
   - Spawns background FreeRTOS tasks (`motor_runner_task`, `input_poller_task`).
2. Update `src/main.cpp`:
   - Initialize Serial and hardware pins.
   - Call `CompositionRoot::setup()` and `CompositionRoot::start()`.
   - Call `vTaskDelete(NULL)` to terminate the Arduino `loop()` task.
3. Remove legacy code:
   - Delete obsolete `src/AviatorClock.h` and `src/AviatorClock.cpp`.
   - Delete legacy libraries under `lib/` (`soc-api`, `soc-esp32`, `soc-mock`, `stepper-api`, `accel-stepper-adapter`, `buttons`, `wifi-manager`, `aviator-clock`, `arduino-mock`).
4. Update `platformio.ini`:
   - Remove `lib_ignore` rules that were working around local library conflicts.
   - Clean up dependencies and include paths.
5. Verify that all GoogleTest test suites pass natively and the hardware build produces a clean binary.

## Verification
- Run `pio test -e google_tests` — 100% passing.
- Run `pio run -e nodemcu-32s` — clean build, exit code 0.
- Verify directory structure matches `docs/ARCHITECTURE.md`.

## Comments
- Implemented `CompositionRoot.h/.cpp` in `src/app/` under namespace `aviator::app`, wiring all domain components, ports, and platform adapters via dependency injection.
- Rewrote `src/main.cpp` to initialize Serial, instantiate `CompositionRoot`, call `setup()` and `start()`, and terminate the loop task with `vTaskDelete(NULL)`.
- Removed legacy `src/AviatorClock.h` and `src/AviatorClock.cpp`.
- Deleted all 9 legacy libraries in `lib/` (`soc-api`, `soc-esp32`, `soc-mock`, `stepper-api`, `accel-stepper-adapter`, `buttons`, `wifi-manager`, `aviator-clock`, `arduino-mock`).
- Cleaned up `platformio.ini`, removing `lib_ignore` and legacy dependencies.
- Added `DigitalInput` & `GpioDigitalInput` in `src/platform/sensor/` and `Logger` in `src/platform/logging/`, eliminating any remaining dependencies on legacy libraries.
- Verified that all 106 GoogleTest unit tests pass with 100% success on the native host.
- Verified that `pio run -e nodemcu-32s` builds cleanly with 0 errors and 0 warnings.

### Code Review Findings: Embedded Standards & Object Lifecycles
1. **Dynamic Instantiation of `AsyncWebServer` upon Operating Mode Changes:**
   - **Finding:** In `WebPortalServer::start()` and `stop()`, `_server` is allocated via `std::make_unique<AsyncWebServer>(_configuration.port)` and destructed via `_server.reset()`. Every transition into and out of Setup Mode performs runtime heap allocations.
   - **Remediation Instructions:**
     - Pre-instantiate the server or maintain it as a persistent member object, calling `_server.begin()` and `_server.end()` instead of reallocating memory on the heap.

2. **Task Deletion during Runtime Shutdown / Mode Switching:**
   - **Finding:** `CompositionRoot::stop()`, `StepperSubsystem::stop()`, and `SetupButtonAdapter::stop()` call `vTaskDelete()` on running tasks. While acceptable for a full teardown, `ARCHITECTURE.md` Section 4 dictates that operational task lifecycles run continuously for the system lifetime and use event notifications / yielding rather than task recreation.
   - **Remediation Instructions:**
     - Keep background tasks (`motor_runner_task`, `setup_btn_task`, `clock_engine_task`) alive; transition them into a sleeping/waiting state using `xTaskNotifyWait` or condition flags rather than destroying and recreating them.

3. **Global CompositionRoot Dynamic Allocation:**
   - **Finding:** In `src/main.cpp`, `globalCompositionRoot = std::make_unique<aviator::app::CompositionRoot>()` allocates the entire CompositionRoot on the heap.
   - **Remediation Instructions:**
     - While permitted under the "once during initialization" exemption, this can be made strictly zero-heap by declaring `static aviator::app::CompositionRoot globalCompositionRoot;` with static storage duration.
