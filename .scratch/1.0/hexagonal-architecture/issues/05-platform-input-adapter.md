# 05 Implement Platform Input Adapter (Single Setup Button)

Status: ready-for-agent
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
