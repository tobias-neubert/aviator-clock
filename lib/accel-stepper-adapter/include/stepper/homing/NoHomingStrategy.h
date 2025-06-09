#pragma once

#include <stepper/api/IHomingStrategy.h>
#include <stepper/api/IStepperMotor.h>

namespace stepper
{
    namespace homing
    {
        class NoHomingStrategy : public stepper::api::IHomingStrategy
        {
        public:
            explicit NoHomingStrategy();
            virtual ~NoHomingStrategy() = default;

            // --- soc::api::IHomingStrategy Interface Implementation ---
            bool beginHoming() override;
            stepper::api::HomingResult updateHoming() override;
            stepper::api::HomingResult getHomingResult() const override;
            void cancelHoming() override;
            void resetStrategy() override;
        };

    } // namespace homing
} // namespace stepper
