#include "buttons/impl/LongPressButton.h"

LongPressButton::LongPressButton(soc::api::ISoc& systemOnChip, soc::api::IDigitalInput& pin, unsigned long longPressTimeMs, unsigned long debounceTimeMs) :
    _systemOnChip(systemOnChip),
    _pin(pin),
    _longPressTimeMs(longPressTimeMs),
    _debounceTimeMs(debounceTimeMs) {}

void LongPressButton::update() {
    bool isPressed = _pin.isActive(); // Assumes active-low configuration (pull-up)

    switch (_state) {
        case State::IDLE:
            if (isPressed) {
                _pressStartTime = _systemOnChip.getTimeProvider().millis();
                _state = State::DEBOUNCING;
            }
            break;

        case State::DEBOUNCING:
            if (_systemOnChip.getTimeProvider().millis() - _pressStartTime > _debounceTimeMs) {
                if (isPressed) {
                    _state = State::PRESSED;
                } else {
                    _state = State::IDLE;
                }
            }
            break;

        case State::PRESSED:
            if (!isPressed) {
                _state = State::RELEASED;
                _shortPressEvent = true; // Register a short press event on release
            } else if (_systemOnChip.getTimeProvider().millis() - _pressStartTime > _longPressTimeMs) {
                _state = State::LONG_PRESS_DETECTED;
                _longPressEvent = true; // Register a long press event
            }
            break;

        case State::LONG_PRESS_DETECTED:
            // Wait for the button to be released
            if (!isPressed) {
                _state = State::IDLE;
            }
            break;

        case State::RELEASED:
            // This state is transient, reset to IDLE immediately.
            // The event flags are consumed by the was...Pressed() methods.
             _state = State::IDLE;
            break;
    }
}

bool LongPressButton::wasShortPressed() {
    if (_shortPressEvent) {
        _shortPressEvent = false; // Consume the event
        return true;
    }
    return false;
}

bool LongPressButton::wasLongPressed() {
    if (_longPressEvent) {
        _longPressEvent = false; // Consume the event
        return true;
    }
    return false;
}
