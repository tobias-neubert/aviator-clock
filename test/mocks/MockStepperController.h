#pragma once

#include "platform/motor/StepperController.h"
#include <gmock/gmock.h>

namespace aviator::test {

class MockStepperController : public aviator::platform::motor::StepperController {
public:
    MOCK_METHOD(void, setEnablePin, (uint8_t enablePin), (override));
    MOCK_METHOD(void, setPinsInverted, (bool directionInvert, bool stepInvert, bool enableInvert), (override));
    MOCK_METHOD(void, enableOutputs, (), (override));
    MOCK_METHOD(void, disableOutputs, (), (override));

    MOCK_METHOD(void, setMaxSpeed, (float speed), (override));
    MOCK_METHOD(void, setAcceleration, (float acceleration), (override));

    MOCK_METHOD(void, moveTo, (long absoluteSteps), (override));
    MOCK_METHOD(void, move, (long relativeSteps), (override));

    MOCK_METHOD(long, getCurrentPosition, (), (const, override));
    MOCK_METHOD(void, setCurrentPosition, (long absoluteSteps), (override));
    MOCK_METHOD(long, distanceToGo, (), (const, override));

    MOCK_METHOD(bool, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

} // namespace aviator::test
