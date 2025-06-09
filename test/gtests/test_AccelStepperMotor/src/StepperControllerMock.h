#pragma once

#include <stepper/api/IStepperController.h>
#include <gmock/gmock.h>

namespace stepper
{
    namespace testing
    {
        class MockStepperController : public stepper::api::IStepperController
        {
        public:
            MOCK_METHOD(void, setEnablePin, (uint8_t enablePin), (override));
            MOCK_METHOD(void, setPinsInverted, (bool dirInvert, bool stepInvert, bool enableInvert), (override));
            MOCK_METHOD(void, enableOutputs, (), (override));
            MOCK_METHOD(void, disableOutputs, (), (override));

            MOCK_METHOD(void, setMaxSpeed, (float speed), (override));
            MOCK_METHOD(void, setAcceleration, (float acceleration), (override));

            MOCK_METHOD(void, moveTo, (long absoluteSteps), (override));
            MOCK_METHOD(void, move, (long relativeSteps), (override));

            MOCK_METHOD(long, getCurrentPosition, (), (override));
            MOCK_METHOD(void, setCurrentPosition, (long absoluteSteps), (override));
            MOCK_METHOD(long, distanceToGo, (), (override));

            MOCK_METHOD(bool, run, (), (override));
            MOCK_METHOD(void, stop, (), (override));
        };
    }
}
