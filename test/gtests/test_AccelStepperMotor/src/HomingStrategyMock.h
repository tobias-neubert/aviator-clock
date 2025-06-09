#pragma once

#include <stepper/api/IHomingStrategy.h>
#include <stepper/api/HomingResult.h>
#include <gmock/gmock.h>

namespace stepper
{
    namespace testing
    {
        class MockHomingStrategy : public stepper::api::IHomingStrategy
        {
        public:
            MOCK_METHOD(bool, beginHoming, (), (override));
            MOCK_METHOD(stepper::api::HomingResult, updateHoming, (), (override));
            MOCK_METHOD(stepper::api::HomingResult, getHomingResult, (), (const, override));
            MOCK_METHOD(void, cancelHoming, (), (override));
            MOCK_METHOD(void, resetStrategy, (), (override));
        };
    }
}
