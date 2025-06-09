#pragma once

#include <stepper/api/IStepperController.h>
#include <stepper/api/IStepperMotor.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/api/StepperMotorState.h>
#include <soc/api/ILogger.h>

// A common invalid pin marker
#ifndef INVALID_PIN
#define INVALID_PIN 0xFF
#endif

namespace stepper
{
    namespace accel
    {
        class AccelStepperMotor : public IStepperMotor
        {
        public:
            AccelStepperMotor(
                stepper::api::IStepperController& stepperController,
                int fullStepsPerRevolution,
                stepper::api::IHomingStrategy& homingStrategy,
                soc::api::ILogger& logger,
                uint8_t enablePin = INVALID_PIN,
                bool enablePinActiveLow = true);

            // --- IStepperMotor Interface Implementation ---
            bool home() override;
            bool isHomingFailed() const override;
            bool isHoming() const override;
            bool needsHoming() const override;
            bool moveToAbsolute(double degreesAbsolute) override;
            bool rotateRelative(double degreesRelative) override;
            void setSpeed(double degreesPerSecond) override;
            void setAcceleration(double degreesPerSecondSquared) override;
            double getCurrentPositionDegrees() const override;
            void enable() override;
            void disable() override;
            stepper::api::StepperMotorState getState() const override;

            // Non-blocking state machine methods
            void update() override;
            bool isBusy() const override;
            void stop() override;

        private:
            stepper::api::IStepperController& _stepperController;
            const int _fullStepsPerRevolution;
            const uint8_t _enablePin;
            stepper::api::IHomingStrategy& _homingStrategy;
            bool _isHoming;

            bool _isHomed;
            float _currentSetSpeedDps;
            float _currentSetAccelerationDps2;

            soc::api::ILogger& _logger;
            stepper::api::StepperMotorState _currentState;

            long degreesToSteps(double degrees) const;
            double stepsToDegrees(long steps) const;
        };
    }
}