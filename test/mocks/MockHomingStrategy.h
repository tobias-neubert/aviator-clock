#pragma once

#include "platform/motor/HomingStrategy.h"
#include <gmock/gmock.h>

namespace aviator::test {

class MockHomingStrategy : public aviator::platform::motor::HomingStrategy {
public:
    MOCK_METHOD(bool, beginHoming, (), (override));
    MOCK_METHOD(aviator::platform::motor::HomingResult, updateHoming, (), (override));
    MOCK_METHOD(aviator::platform::motor::HomingResult, getHomingResult, (), (const, override));
    MOCK_METHOD(void, cancelHoming, (), (override));
    MOCK_METHOD(void, resetStrategy, (), (override));
};

} // namespace aviator::test
