#pragma once

#include <gmock/gmock.h>
#include "domain/ClockInputEvents.h"

namespace aviator::test {

class MockClockInputEvents : public domain::ClockInputEvents {
public:
    MOCK_METHOD(void, onSetupButtonPressed, (), (override));
    MOCK_METHOD(void, onSetupButtonLongPressed, (), (override));
    MOCK_METHOD(void, onSetupButtonFactoryReset, (), (override));
};

} // namespace aviator::test
