# 05 Implement Platform Input Adapter (Single Setup Button)

Status: closed
Blocked by: 02

## Context
With the simplified Version 1.0 requirements, rotary encoders, DIP switches, and mode buttons are eliminated. Hardware user input is simplified to a single push button (`PIN_SETUP_BUTTON` on GPIO 15). We need a dedicated, non-blocking platform adapter that monitors this button and dispatches events to the domain port `ClockInputEvents`.

## Requirements
1. Create `src/platform/input/SetupButtonAdapter.h/.cpp` in namespace `aviator::platform::input`:
   - Monitors `PIN_SETUP_BUTTON` using active-low digital input logic and software debouncing.
   - Detects short press: dispatches `onSetupButtonPressed()` (used to dismiss active alarm).
   - Detects 3-second long press: dispatches `onSetupButtonLongPressed()` (toggle Setup Mode AP + Web Portal).
   - Detects 10-second hold: dispatches `onSetupButtonFactoryReset()` (erase NVS credentials and reboot).
   - Runs either in a lightweight FreeRTOS polling task (20 ms interval) or via GPIO interrupt.
   - Forwards detected actions directly to the domain port `ClockInputEvents`.
2. Implement unit tests for button debouncing and timing in GoogleTest.

## Verification
- Unit test short press, 3s long press, and 10s factory reset detection in GoogleTest.
- Verify hardware compilation with `pio run -e nodemcu-32s`.

## Comments
- Implemented `SetupButtonAdapter` in `src/platform/input/SetupButtonAdapter.h/.cpp` in namespace `aviator::platform::input`.
- Encapsulated active-low digital input logic (GPIO 15 default) with configurable software debouncing (50 ms default), long press threshold (3000 ms), and factory reset threshold (10000 ms).
- Implemented FreeRTOS polling task lifecycle (`start()`, `stop()`, `isRunning()`) with 20 ms polling interval.
- Implemented deterministic gesture detection:
  - Short press (< 3s, debounced) dispatches `ClockInputEvents::onSetupButtonPressed()`.
  - 3-second long press (3s - 10s, debounced) dispatches `ClockInputEvents::onSetupButtonLongPressed()`.
  - 10-second hold (>= 10s) immediately dispatches `ClockInputEvents::onSetupButtonFactoryReset()` while held, suppressing subsequent release gestures.
- Added `MockClockInputEvents` mock in `test/mocks/MockClockInputEvents.h`.
- Created comprehensive GoogleTest suite in `test/gtests/test_input/src/test_SetupButtonAdapter.cpp` (14 unit tests covering short press, bounce immunity, release chatter rejection, threshold transitions, 10s hold, custom pin/time injection, active-high configuration, zero-debounce configuration, and task lifecycle).
- Updated `platformio.ini` to include `src/platform/input/` and `gtests/test_input`.
- Verified 100% test pass (109/109 test cases) with `pio test -e google_tests` and clean firmware build with `pio run -e nodemcu-32s`.
