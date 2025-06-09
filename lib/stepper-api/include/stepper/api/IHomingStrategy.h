#pragma once

#include <stepper/api/HomingResult.h>
using stepper::api::HomingResult;

namespace stepper
{
    namespace api
    {
        class IHomingStrategy
        {
        public:
            virtual ~IHomingStrategy() = default;

            /**
             * @brief Initiates the homing sequence.
             * This method should set up the motor for homing (e.g., low speed, appropriate acceleration)
             * and start the first phase of the homing movement.
             * It should be called by the IStepperMotor's home() method.
             *
             * @return True if the homing sequence was successfully initiated,
             * false otherwise (e.g., if the strategy determines it cannot start).
             */
            virtual bool beginHoming() = 0;

            /**
             * @brief Called repeatedly by the IStepperMotor's update() method when homing is active.
             * This method contains the core state machine logic for the specific homing strategy.
             * It will command the motor (via the IStepperMotor reference it holds) and check sensors
             * (via the IDigitalInput reference it holds, if applicable).
             *
             * @return The current HomingResult of the operation. HomingResult::SUCCESS or any of the
             * FAILURE_... types indicate that the homing process has completed.
             * HomingResult::IN_PROGRESS means it should continue to be called.
             */
            virtual HomingResult updateHoming() = 0;

            /**
             * @brief Gets the last reported result or current status of the homing operation.
             * @return The current HomingResult.
             */
            virtual HomingResult getHomingResult() const = 0;

            /**
             * @brief Cancels an ongoing homing operation.
             * The strategy should attempt to stop the motor safely.
             */
            virtual void cancelHoming() = 0;

            /**
             * @brief Resets the internal state of the strategy, making it ready for a new homing attempt.
             * This might be called after a successful or failed homing if a re-home is desired.
             */
            virtual void resetStrategy() = 0;
        };

    } // namespace api
} // namespace stepper