#pragma once

#include <cstdint>

namespace stepper
{
    namespace api
    {
        class IStepperController
        {
        public:
            virtual ~IStepperController() = default;

            virtual void setEnablePin(uint8_t enablePin) = 0;
            virtual void setPinsInverted(bool dirInvert, bool stepInvert, bool enableInvert) = 0;
            virtual void enableOutputs() = 0;
            virtual void disableOutputs() = 0;

            virtual void setMaxSpeed(float speed) = 0;
            virtual void setAcceleration(float acceleration) = 0;

            virtual void moveTo(long absoluteSteps) = 0;
            virtual void move(long relativeSteps) = 0;

            virtual long getCurrentPosition() = 0;
            virtual void setCurrentPosition(long absoluteSteps) = 0;
            virtual long distanceToGo() = 0;

            virtual bool run() = 0;
            virtual void stop() = 0;
        };
    }
}
