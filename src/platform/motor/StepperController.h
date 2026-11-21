#pragma once

#include <cstdint>

namespace aviator::platform::motor {

/**
 * @brief Abstract interface for low-level stepper pulse generation controllers.
 * 
 * Defines the contract required by stepper actuators to control pulse generation,
 * directional control, output enablement, acceleration ramping, and target position tracking.
 */
class StepperController {
public:
    virtual ~StepperController() = default;

    /**
     * @brief Configures the hardware pin used to enable or disable the motor coil outputs.
     */
    virtual void setEnablePin(uint8_t enablePin) = 0;

    /**
     * @brief Inverts the active electrical levels for direction, step pulse, or enable lines.
     */
    virtual void setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert) = 0;

    /**
     * @brief Energizes motor coils by driving the driver's enable pin active.
     */
    virtual void enableOutputs() = 0;

    /**
     * @brief De-energizes motor coils to reduce power dissipation and temperature when idle.
     */
    virtual void disableOutputs() = 0;

    /**
     * @brief Sets the maximum stepping rate in steps per second.
     */
    virtual void setMaxSpeed(float speed) = 0;

    /**
     * @brief Sets the acceleration rate used for speed ramping in steps per second squared.
     */
    virtual void setAcceleration(float acceleration) = 0;

    /**
     * @brief Commands the controller to move to an absolute step position.
     */
    virtual void moveTo(long absoluteSteps) = 0;

    /**
     * @brief Commands the controller to move a relative distance from its current step count.
     */
    virtual void move(long relativeSteps) = 0;

    /**
     * @brief Retrieves the current accumulated step position of the controller.
     */
    virtual long getCurrentPosition() const = 0;

    /**
     * @brief Overrides the internal position counter without executing physical movement.
     * Essential for zero calibration and forward wrap-around resets across 360 degrees.
     */
    virtual void setCurrentPosition(long absoluteSteps) = 0;

    /**
     * @brief Calculates the remaining steps to reach the commanded target position.
     */
    virtual long distanceToGo() const = 0;

    /**
     * @brief Processes step generation if the calculated step interval has elapsed.
     * @return True if a step pulse was output or motion is ongoing, false if stopped at target.
     */
    virtual bool run() = 0;

    /**
     * @brief Initiates deceleration to stop the motor at its current trajectory.
     */
    virtual void stop() = 0;
};

} // namespace aviator::platform::motor
