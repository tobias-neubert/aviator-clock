#pragma once

/**
 * @file IButton.h
 * @brief Defines the interface for a generic button.
 *
 * This abstract class provides a contract for button implementations,
 * allowing for different types of button interactions (e.g., short press,
 * long press) to be handled polymorphically.
 */
class IButton {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IButton() = default;

    /**
     * @brief Updates the button's internal state.
     *
     * This method should be called repeatedly in the main loop or a dedicated task.
     * It handles debouncing, timing, and state transitions.
     */
    virtual void update() = 0;

    /**
     * @brief Checks if a short press event has occurred.
     *
     * @return true if a short press was detected since the last check, false otherwise.
     * This state is typically consumed upon reading (i.e., it will return false
     * on subsequent calls until a new event occurs).
     */
    virtual bool wasShortPressed() = 0;

    /**
     * @brief Checks if a long press event has occurred.
     *
     * @return true if a long press was detected since the last check, false otherwise.
     * This state is typically consumed upon reading.
     */
    virtual bool wasLongPressed() = 0;
};
