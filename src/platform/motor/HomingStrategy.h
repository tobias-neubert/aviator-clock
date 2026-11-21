#pragma once

namespace aviator::platform::motor {

/**
 * @brief Status outcome of a homing procedure.
 */
enum class HomingResult {
    NOT_YET_STARTED,
    IN_PROGRESS,
    SUCCESS,
    FAILURE_TIMEOUT,
    FAILURE_MOTOR_ERROR,
    CANCELLED
};

/**
 * @brief Abstract interface for dial zero-point homing strategies.
 */
class HomingStrategy {
public:
    virtual ~HomingStrategy() = default;

    /**
     * @brief Initiates the homing sequence from its beginning phase.
     * @return True if the strategy started successfully, false if hardware preconditions failed.
     */
    virtual bool beginHoming() = 0;

    /**
     * @brief Steps the homing state machine and queries sensor readings.
     * Must be called repeatedly within the motor runner loop until a terminal state is reached.
     * @return Current status of the homing procedure.
     */
    virtual HomingResult updateHoming() = 0;

    /**
     * @brief Retrieves the latest homing result.
     */
    virtual HomingResult getHomingResult() const = 0;

    /**
     * @brief Aborts an in-progress homing sequence immediately.
     */
    virtual void cancelHoming() = 0;

    /**
     * @brief Resets internal phase tracking to allow starting a fresh homing cycle.
     */
    virtual void resetStrategy() = 0;
};

} // namespace aviator::platform::motor
