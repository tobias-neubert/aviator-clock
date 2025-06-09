#pragma once

namespace stepper
{
    namespace api
    {
        enum class StepperMotorState
        {
            IDLE,
            MOVING,
            HOMING_IN_PROGRESS,
            HOMING_FAILED
        };
    }
}
