#pragma once

namespace aviator::domain {

/**
 * @brief Abstract outbound port for controlling a dial hand actuator.
 */
class HandActuator {
public:
    virtual ~HandActuator() = default;

    /**
     * @brief Commands the actuator to move towards the specified target angle in degrees.
     * @param degrees Target angle in degrees.
     */
    virtual void setTargetAngle(double degrees) = 0;

    /**
     * @brief Gets the current target angle commanded to the actuator.
     * @return Target angle in degrees.
     */
    virtual double getTargetAngle() const = 0;

    /**
     * @brief Forces the internal position counter of the actuator to the given angle.
     * Used after forward wrap-around jumps across 360° to reset the datum to 0°.
     * @param degrees New position in degrees.
     */
    virtual void setCurrentAngle(double degrees) = 0;

    /**
     * @brief Gets the current estimated or physical angle of the actuator.
     * @return Current angle in degrees.
     */
    virtual double getCurrentAngle() const = 0;

    /**
     * @brief Initiates the physical homing sequence towards the zero-point sensor.
     * @return True if homing was initiated successfully, false otherwise.
     */
    virtual bool home() = 0;

    /**
     * @brief Checks if the hand is homed and calibrated to its zero point.
     * @return True if homed, false otherwise.
     */
    virtual bool isHomed() const = 0;

    /**
     * @brief Checks if the actuator is currently in motion.
     * @return True if moving, false if idle at target.
     */
    virtual bool isBusy() const = 0;

    /**
     * @brief Immediately stops all actuator motion.
     */
    virtual void stop() = 0;
};

} // namespace aviator::domain
