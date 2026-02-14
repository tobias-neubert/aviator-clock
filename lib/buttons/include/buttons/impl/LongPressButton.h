#pragma once

#include <buttons/api/IButton.h>
#include <soc/api/IDigitalInput.h>
#include <soc/api/ISoc.h>

/**
 * @file LongPressButton.h
 * @brief An implementation of IButton that detects short and long presses.
 *
 * This class uses the abstract ISoc and IDigitalInput interfaces for hardware
 * interaction, making it platform-independent and testable.
 */
class LongPressButton : public IButton {
public:
    /**
     * @brief Construct a new Long Press Button object.
     *
     * @param systemOnChip A reference to the SoC object, used to get time.
     * @param pin A reference to the abstract digital input pin.
     * @param longPressTimeMs The duration in milliseconds to qualify as a long press.
     * @param debounceTimeMs The debounce delay in milliseconds.
     */
    LongPressButton(soc::api::ISoc& systemOnChip, soc::api::IDigitalInput& pin, unsigned long longPressTimeMs = 3000, unsigned long debounceTimeMs = 50);

    /**
     * @brief Updates the button state machine. Must be called frequently.
     */
    void update() override;

    /**
     * @brief Checks for a short press event.
     * @return true if a short press occurred, false otherwise. Consumes the event.
     */
    bool wasShortPressed() override;

    /**
     * @brief Checks for a long press event.
     * @return true if a long press occurred, false otherwise. Consumes the event.
     */
    bool wasLongPressed() override;

private:
    // Enum to represent the button's state machine
    enum class State {
        IDLE,
        DEBOUNCING,
        PRESSED,
        LONG_PRESS_DETECTED,
        RELEASED
    };

    soc::api::ISoc& _systemOnChip;
    soc::api::IDigitalInput& _pin;
    const unsigned long _longPressTimeMs;
    const unsigned long _debounceTimeMs;

    State _state = State::IDLE;
    unsigned long _pressStartTime = 0;
    bool _shortPressEvent = false;
    bool _longPressEvent = false;
};
