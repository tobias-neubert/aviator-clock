#include <stepper/homing/NoHomingStrategy.h>

namespace stepper
{
    namespace homing
    {
        NoHomingStrategy::NoHomingStrategy() {}

        bool NoHomingStrategy::beginHoming()
        {
            return true;
        }

        stepper::api::HomingResult NoHomingStrategy::updateHoming()
        {
            return stepper::api::HomingResult::SUCCESS;
        }

        stepper::api::HomingResult NoHomingStrategy::getHomingResult() const
        {
            return stepper::api::HomingResult::SUCCESS;
        }

        void NoHomingStrategy::cancelHoming()
        {
        }

        void NoHomingStrategy::resetStrategy()
        {
        }

    } // namespace strategy
} // namespace soc