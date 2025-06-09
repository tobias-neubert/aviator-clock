#pragma once
namespace stepper
{
    namespace api
    {
        // Detailed status values needed because we are implementing
        // all components attached to our soc as state machines
        // that are frequently called by the main loop to update their
        // state.
        // TODO: Maybe blocking is better here for precision?
        enum class HomingResult
        {
            NOT_YET_STARTED,     // Homing has not been initiated
            INITIATION_SUCCESS,  // beginHoming() was successful, process starting
            INITIATION_FAILURE,  // beginHoming() failed (e.g., motor busy)
            IN_PROGRESS,         // Homing is actively running
            SUCCESS,             // Homing completed successfully
            FAILURE_TIMEOUT,     // Homing failed due to timeout
            FAILURE_NO_TRIGGER,  // Homing failed because sensor/stall was not detected within limits
            FAILURE_MOTOR_ERROR, // Homing failed due to an issue with motor movement
            CANCELLED            // Homing was cancelled
        };
    }
}
