#pragma once

#include <stepper/api/IStepperController.h>
#include <AccelStepper.h>

using stepper::api::IStepperController;

namespace stepper
{
    namespace accel
    {
        class AccelStepperWrapper : public IStepperController
        {
        private:
            AccelStepper _accelStepper;

        public:
            AccelStepperWrapper(uint8_t stepPin,
                                uint8_t dirPin,
                                AccelStepper::MotorInterfaceType interface = AccelStepper::DRIVER)
                : _accelStepper(interface, stepPin, dirPin) {}

            void setEnablePin(uint8_t enablePin) override
            {
                _accelStepper.setEnablePin(enablePin);
            }

            void setPinsInverted(bool dirInvert, bool stepInvert, bool enableInvert) override
            {
                _accelStepper.setPinsInverted(dirInvert, stepInvert, enableInvert);
            }

            void enableOutputs() override
            {
                _accelStepper.enableOutputs();
            }

            void disableOutputs() override
            {
                _accelStepper.disableOutputs();
            }

            void setMaxSpeed(float speed) override
            {
                _accelStepper.setMaxSpeed(speed);
            }

            void setAcceleration(float acceleration) override
            {
                _accelStepper.setAcceleration(acceleration);
            }

            void moveTo(long absoluteSteps) override
            {
                _accelStepper.moveTo(absoluteSteps);
            }

            void move(long relativeSteps) override
            {
                _accelStepper.move(relativeSteps);
            }

            long getCurrentPosition() override
            {
                return _accelStepper.currentPosition();
            }

            void setCurrentPosition(long absoluteSteps) override
            {
                _accelStepper.setCurrentPosition(absoluteSteps);
            }

            long distanceToGo() override
            {
                return _accelStepper.distanceToGo();
            }

            bool run() override
            {
                return _accelStepper.run();
            }

            void stop() override
            {
                _accelStepper.stop();
            }
        };
    }
}