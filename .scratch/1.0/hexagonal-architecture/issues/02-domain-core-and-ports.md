# 02 Extract Pure Domain Core and Abstract Ports into src/domain/

Status: ready-for-agent
Blocked by: 01

## Context
Domain logic is currently trapped inside `lib/aviator-clock/` and mixed with hardware calls and FreeRTOS task handling. We need to create the pure domain layer under `src/domain/` with zero dependencies on FreeRTOS, Arduino, or hardware headers.

## Requirements
1. Create `src/domain/` and namespace `aviator::domain`.
2. Move and refine domain entities and arithmetic:
   - `Time.h` and `Alarm.h` (plain data structs).
   - `DialGeometry.h/.cpp`: Pure math translating hours/minutes/seconds to degrees across the 330.0° active arc, with deterministic forward clockwise wrap-around jumping (59->0 and 11->0).
   - `TimeEditor.h/.cpp`: Unit wrapping and 12h/24h arithmetic, calculating AM/PM second hand positions.
   - `OperatingStateMachine.h/.cpp`: Pure state machine managing `NormalRunning`, `AlarmSetup`, and `WifiSetup` based on domain events.
3. Define abstract domain ports (pure abstract base classes with NO 'I' prefix):
   - `HandActuator.h`: `setTargetAngle(double degrees)`, `home()`, `isHomed()`, `isBusy()`, `stop()`.
   - `AlarmStorage.h`: `loadAlarm(uint8_t slot)`, `saveAlarm(uint8_t slot, const Time& time)`.
   - `TimeSource.h`: `now()`, `asTimeComponents(TimeComponents& components)`.
   - `ClockInputEvents.h`: `onModeButtonPressed()`, `onSetupButtonPressed()`, `onSlotChanged(uint8_t slot)`, `onEncoderRotated(EncoderId id, int32_t delta)`.
   - `SystemGateway.h`: `restart()`, `resetWifiCredentials()`.
4. Update native GoogleTests in `test/gtests/` to target `src/domain/` directly using lightweight test doubles.

## Verification
- Run `pio test -e google_tests` to verify that all domain unit tests pass against `src/domain/`.

## Comments
