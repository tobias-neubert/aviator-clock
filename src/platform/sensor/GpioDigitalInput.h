#pragma once

#include <cstdint>
#include "platform/sensor/DigitalInput.h"

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#endif

namespace aviator::platform::sensor {

/**
 * @brief Concrete hardware digital input adapter reading a micro-controller GPIO pin.
 */
class GpioDigitalInput : public DigitalInput {
public:
    explicit GpioDigitalInput(uint8_t pin, bool activeLow = true)
        : _pin(pin), _activeLow(activeLow) {
#if defined(ARDUINO) || defined(ESP32)
        pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
#endif
    }

    bool isActive() const override {
#if defined(ARDUINO) || defined(ESP32)
        const int pinState = digitalRead(_pin);
        return _activeLow ? (pinState == LOW) : (pinState == HIGH);
#else
        return false;
#endif
    }

    uint8_t getPin() const {
        return _pin;
    }

    bool isActiveLow() const {
        return _activeLow;
    }

private:
    uint8_t _pin;
    bool _activeLow;
};

} // namespace aviator::platform::sensor
