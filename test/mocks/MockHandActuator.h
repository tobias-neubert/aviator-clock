#pragma once

#include <gmock/gmock.h>
#include "domain/HandActuator.h"

namespace aviator::test {

class MockHandActuator : public domain::HandActuator {
public:
    MOCK_METHOD(void, setTargetAngle, (double degrees), (override));
    MOCK_METHOD(double, getTargetAngle, (), (const, override));
    MOCK_METHOD(void, setCurrentAngle, (double degrees), (override));
    MOCK_METHOD(double, getCurrentAngle, (), (const, override));
    MOCK_METHOD(bool, home, (), (override));
    MOCK_METHOD(bool, isHomed, (), (const, override));
    MOCK_METHOD(bool, isBusy, (), (const, override));
    MOCK_METHOD(void, stop, (), (override));
};

} // namespace aviator::test
