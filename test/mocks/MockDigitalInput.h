#pragma once

#include <gmock/gmock.h>
#include "platform/sensor/DigitalInput.h"

namespace aviator::test {

/**
 * @brief GoogleMock test double for aviator::platform::sensor::DigitalInput.
 */
class MockDigitalInput : public platform::sensor::DigitalInput {
public:
    explicit MockDigitalInput(uint8_t pin = 0, bool activeLow = true)
        : _pin(pin), _activeLow(activeLow) {}

    MOCK_METHOD(bool, isActive, (), (const, override));

    void setActive(bool active) {
        ON_CALL(*this, isActive()).WillByDefault(::testing::Return(active));
    }

private:
    uint8_t _pin;
    bool _activeLow;
};

} // namespace aviator::test
