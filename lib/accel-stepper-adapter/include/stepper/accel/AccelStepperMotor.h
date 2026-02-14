#pragma once

#include <stepper/api/IStepperController.h>
#include <stepper/api/IStepperMotor.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/api/StepperMotorState.h>
#include <soc/api/Logger.h>

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
                soc::api::Logger& logger,
                stepper::api::IStepperController& stepperController,
                int fullStepsPerRevolution,
                stepper::api::IHomingStrategy& homingStrategy,
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
            void setCurrentPositionDegrees(double degrees) override;
            void enable() override;
            void disable() override;
            stepper::api::StepperMotorState getState() const override;

            // Non-blocking state machine methods
            void update() override;
            bool isBusy() const override;
            void stop() override;
            void stopBlocking() override;

        private:
            soc::api::Logger& _logger;
            stepper::api::IStepperController& _stepperController;
            const int _fullStepsPerRevolution;
            const uint8_t _enablePin;
            stepper::api::IHomingStrategy& _homingStrategy;
            bool _isHoming;

            bool _isHomed;
            float _currentSetSpeedDps;
            float _currentSetAccelerationDps2;

            
            stepper::api::StepperMotorState _currentState;

            long degreesToSteps(double degrees) const;
            double stepsToDegrees(long steps) const;
        };
    }
}