# 05 Implement Platform Input Adapter (Buttons, Encoders, DIP Switches)

Status: ready-for-agent
Blocked by: 02

## Context
User input processing is currently handled through `lib/buttons` (`LongPressButton`, `SwitchGroup`), `madhephaestus/ESP32Encoder`, and a polling loop in `AviatorClock.cpp`. We need a dedicated, non-blocking platform adapter that monitors all input hardware and dispatches events to the domain port `ClockInputEvents`.

## Requirements
1. Create `src/platform/input/` in namespace `aviator::platform::input`.
2. Implement `RotaryEncoder`:
   - Self-contained quadrature decoder using GPIO pin-change interrupts or PCNT driver, with zero external library dependencies.
   - Replaces `madhephaestus/ESP32Encoder`.
3. Implement `InputPollerTask`:
   - Runs a single FreeRTOS task at a 20 ms polling interval.
   - Reads the 3-bit DIP switches and detects changes in selected alarm slot.
   - Updates and debounces pushbuttons (Mode button, Setup button), detecting short and long presses (3s for mode/setup toggle, 10s for factory reset).
   - Polls encoder deltas for hours and minutes.
   - Forwards detected actions directly to the domain port `ClockInputEvents`.
   - Never blocks on motor motion or network I/O.

## Verification
- Unit test button debouncing, long-press detection, and DIP switch binary decoding in GoogleTest.
- Verify hardware compilation with `pio run -e nodemcu-32s`.

## Comments
