#pragma once

#include "StepperMotorState.h"

/**
 * @brief Defines an abstract interface for non-blocking control of a stepper motor.
 *
 * This interface provides high-level operations for motor control.
 * Movement commands are non-blocking; an `update()` method must be called
 * frequently to process motor movements and state changes.
 *
 * Units are generally in degrees, degrees per second, and degrees per second squared.
 */
class IStepperMotor
{
public:
    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~IStepperMotor() = default;

    /**
     * @brief Initiates a "soft" homing sequence, defining the current position as zero.
     * For physical homing, this method would need to be expanded or a new one created
     * that works with the state machine and update() method.
     * This current implementation is non-blocking and immediate.
     *
     * @return True if the homing command was accepted (always true for this soft home), false otherwise.
     */
    virtual bool home() = 0;

    virtual bool isHomingFailed() const = 0;

    virtual bool isHoming() const = 0;

    /**
     * @brief Checks if the motor requires homing (or if soft home has been performed).
     * @return True if homing is needed, false if the motor has a known reference position.
     */
    virtual bool needsHoming() const = 0;

    /**
     * @brief Commands the motor to move to an absolute angular position.
     * This is a non-blocking call. `update()` must be called to perform the movement.
     * Assumes the motor has been homed. 0 degrees is the homed position.
     *
     * @param degreesAbsolute The target absolute angular position in degrees.
     * @return True if the move command was accepted (e.g., not busy, homed), false otherwise.
     */
    virtual bool moveToAbsolute(double degreesAbsolute) = 0;

    /**
     * @brief Commands the motor to rotate by a relative number of degrees from its current position.
     * This is a non-blocking call. `update()` must be called to perform the movement.
     *
     * @param degreesRelative The number of degrees to rotate. Positive for one direction, negative for the other.
     * @return True if the rotation command was accepted (e.g., not busy), false otherwise.
     */
    virtual bool rotateRelative(double degreesRelative) = 0;

    /**
     * @brief Sets the target speed for subsequent movements.
     * @param degreesPerSecond The desired maximum speed in degrees per second. Must be positive.
     */
    virtual void setSpeed(double degreesPerSecond) = 0;

    /**
     * @brief Sets the acceleration used for ramping speed up and down during movements.
     * @param degreesPerSecondSquared The desired acceleration in degrees per second squared. Must be positive.
     */
    virtual void setAcceleration(double degreesPerSecondSquared) = 0;

    /**
     * @brief Gets the current estimated angular position of the motor in degrees.
     * This position is relative to the homed (zero) position.
     * Throws std::runtime_error if the motor has not been homed.
     *
     * @return The current angular position in degrees.
     * @throws std::runtime_error if needsHoming() is true.
     */
    virtual double getCurrentPositionDegrees() const = 0;

    virtual stepper::api::StepperMotorState getState() const = 0;

    /**
     * @brief Enables the motor driver, supplying power to the coils.
     */
    virtual void enable() = 0;

    /**
     * @brief Disables the motor driver, cutting power to the coils.
     */
    virtual void disable() = 0;

    /**
     * @brief Updates the motor's state and processes movement.
     * This method MUST be called frequently and repeatedly in the main application loop
     * for the motor to move and for its state to be managed correctly.
     */
    virtual void update() = 0;

    /**
     * @brief Checks if the motor is currently performing an operation (e.g., moving to a target).
     * @return True if the motor is busy, false if it is idle.
     */
    virtual bool isBusy() const = 0;

    /**
     * @brief Commands the motor to initiate a stop sequence.
     * The motor will decelerate to a stop. This is a non-blocking call;
     * `update()` will handle the deceleration process.
     */
    virtual void stop() = 0;
};
