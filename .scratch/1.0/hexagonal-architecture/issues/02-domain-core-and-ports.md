# 02 Extract Pure Domain Core and Abstract Ports into src/domain/

Status: closed
Blocked by: 01

## Context
Domain logic is currently trapped inside `lib/aviator-clock/` and mixed with hardware calls and FreeRTOS task handling. Under the simplified Version 1.0 architecture, we need to create the pure domain layer under `src/domain/` with zero dependencies on FreeRTOS, Arduino, or hardware headers.

## Requirements
1. Create `src/domain/` and namespace `aviator::domain`.
2. Move and refine domain entities and arithmetic:
   - `Time.h`: Plain data struct `{ hours, minutes, seconds }`.
   - `Alarm.h`: Plain data struct `{ Time time, uint16_t soundTrackId, bool enabled }`.
   - `DialGeometry.h/.cpp`: Pure math translating hours/minutes/seconds to degrees across the 330.0° active arc, with deterministic forward clockwise wrap-around jumping (59->0 and 11->0).
   - `AlarmCoordinator.h/.cpp`: Logic checking current time against the configured alarm, triggering and stopping the alarm.
   - `OperatingStateMachine.h/.cpp`: Pure state machine managing `Initial`, `NormalRunning`, and `SetupMode` based on domain events.
3. Define abstract domain ports (pure abstract base classes with NO 'I' prefix):
   - `HandActuator.h`: `setTargetAngle(double degrees)`, `home()`, `isHomed()`, `isBusy()`, `stop()`.
   - `AlarmStorage.h`: `loadAlarm()`, `saveAlarm(const Alarm& alarm)`.
   - `TimeSource.h`: `now()`, `asTimeComponents(TimeComponents& components)`.
   - `AudioPlayer.h`: `playTrack(uint16_t trackNumber)`, `stop()`, `setVolume(uint8_t volume)`.
   - `ClockInputEvents.h`: `onSetupButtonPressed()`, `onSetupButtonLongPressed()`, `onSetupButtonFactoryReset()`.
   - `ClockConfigurationApi.h`: Queries and updates for alarm configuration, Wi-Fi status, and system status used by web portal.
   - `SystemGateway.h`: `restart()`, `resetWifiCredentials()`.
4. Create native GoogleTests in `test/gtests/` to target `src/domain/` directly using lightweight test doubles.

## Verification
- Run `pio test -e google_tests` to verify that all domain unit tests pass against `src/domain/`.

## Comments
- Implemented pure domain entities and services in `src/domain/` under `aviator::domain`: `Time.h`, `Alarm.h`, `DialGeometry.h/.cpp`, `AlarmCoordinator.h/.cpp`, `OperatingStateMachine.h/.cpp`, `ClockEngine.h/.cpp`.
- Implemented pure abstract domain ports with no 'I' prefix: `HandActuator.h`, `AlarmStorage.h`, `TimeSource.h`, `AudioPlayer.h`, `ClockInputEvents.h`, `ClockConfigurationApi.h`, `SystemGateway.h`.
- Created native GoogleMock/GoogleTest test doubles under `test/mocks/` and unit test suites under `test/gtests/test_domain/`.
- Updated `platformio.ini` to build `src/domain/` into `env:google_tests` with `test_build_src = yes`.
- Verified 100% test pass (48/48 test cases) with `pio test -e google_tests` and clean firmware build with `pio run -e nodemcu-32s`.
