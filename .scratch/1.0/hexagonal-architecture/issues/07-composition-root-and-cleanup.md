# 07 Assemble CompositionRoot, Migrate main.cpp, and Remove Legacy lib/

Status: ready-for-agent
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
