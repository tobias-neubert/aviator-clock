#pragma once

#include <stepper/api/IHomingStrategy.h>
#include <stepper/api/IStepperController.h>
#include <soc/api/IDigitalInput.h>
#include <soc/api/Logger.h>

namespace stepper
{
    namespace homing
    {
        class PreciseOneWayHomingStrategy : public stepper::api::IHomingStrategy
        {
        public:
            struct Config
            {
                float homingSpeed;              // Speed for moving towards the switch (steps/sec)
                float homingAcceleration;       // Acceleration for homing (steps/sec^2)
                long maxHomingTravel;           // Max distance before timeout (must be positive)
                int moveDirection;              // +1 for clockwise, -1 for counter-clockwise
                long backOffDistance;           // Steps to move away from the switch if starting on it
            };

            PreciseOneWayHomingStrategy(
                soc::api::Logger &logger,
                stepper::api::IStepperController &stepperController,
                soc::api::IDigitalInput &limitSwitch,
                const Config &config);

            // Inherited via IHomingStrategy
            bool beginHoming() override;
            stepper::api::HomingResult updateHoming() override;
            stepper::api::HomingResult getHomingResult() const override;
            void cancelHoming() override;
            void resetStrategy() override;

        private:
            enum class Phase
            {
                NOT_STARTED,
                MOVING_OFF_SWITCH,
                SEEKING_SWITCH,
                DECELERATING,
                DONE
            };

            void enterPhase(Phase newPhase);

            soc::api::Logger &_logger;
            stepper::api::IStepperController &_stepperController;
            soc::api::IDigitalInput &_limitSwitch;
            const Config _config;
            Phase _phase;
            stepper::api::HomingResult _result;
        };
    }
}