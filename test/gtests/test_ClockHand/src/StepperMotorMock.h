#pragma once

#include <gmock/gmock.h>
#include <stepper/api/IStepperMotor.h>
#include <stepper/api/HomingResult.h>
#include <stepper/api/StepperMotorState.h>

/**
 * @brief Mock implementation of IStepperMotor for Google Test.
 */
class StepperMotorMock : public IStepperMotor
{
public:
    MOCK_METHOD(bool, home, (), (override));
    MOCK_METHOD(bool, isHomingFailed, (), (const, override));
    MOCK_METHOD(bool, isHoming, (), (const, override));
    MOCK_METHOD(bool, needsHoming, (), (const, override));
    MOCK_METHOD(bool, moveToAbsolute, (double degreesAbsolute), (override));
    MOCK_METHOD(bool, rotateRelative, (double degreesRelative), (override));
    MOCK_METHOD(void, setSpeed, (double degreesPerSecond), (override));
    MOCK_METHOD(void, setAcceleration, (double degreesPerSecondSquared), (override));
    MOCK_METHOD(double, getCurrentPositionDegrees, (), (const, override));
    MOCK_METHOD(void, setCurrentPositionDegrees, (double degrees), (override));
    MOCK_METHOD(stepper::api::StepperMotorState, getState, (), (const, override));
    MOCK_METHOD(void, enable, (), (override));
    MOCK_METHOD(void, disable, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, isBusy, (), (const, override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, stopBlocking, (), (override));
};
