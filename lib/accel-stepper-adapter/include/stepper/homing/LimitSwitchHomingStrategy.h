#pragma once

#include <stepper/api/IHomingStrategy.h>
#include <stepper/api/IStepperController.h>
#include <soc/api/IDigitalInput.h>
#include <soc/api/ILogger.h>

namespace stepper
{
    namespace homing
    {
        class LimitSwitchHomingStrategy : public stepper::api::IHomingStrategy
        {
        public:
            struct Config
            {
                long homingSpeedStepsPerSec;          // Speed for moving towards the switch
                long homingAccelerationStepsPerSecSq; // Acceleration for homing
                long maxHomingTravelSteps;            // Max distance before timeout (must be positive)
                int moveDirectionSign;                // +1 or -1: direction to move towards the switch
            };

            LimitSwitchHomingStrategy(stepper::api::IStepperController &stepperController,
                                      soc::api::IDigitalInput &limitSwitch,
                                      const Config &config,
                                      soc::api::ILogger& logger);

            bool beginHoming() override;
            stepper::api::HomingResult updateHoming() override;
            stepper::api::HomingResult getHomingResult() const override;
            void cancelHoming() override;
            void resetStrategy() override;

        private:
            enum class HomingPhase
            {
                NOT_STARTED,
                MOVING_TOWARDS_SWITCH,
                DECELERATING_ON_SWITCH,
                TIMED_OUT
            };

            stepper::api::IStepperController &_stepperController;
            soc::api::IDigitalInput &_limitSwitch;
            const Config _config;

            soc::api::ILogger& _logger;

            HomingPhase _currentPhase;
            stepper::api::HomingResult _currentHomingResult;
        };

    } // namespace homing
} // namespace stepper