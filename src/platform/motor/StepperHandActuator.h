#pragma once

#include "domain/HandActuator.h"
#include "platform/logging/Logger.h"
#include "platform/motor/HomingStrategy.h"
#include "platform/motor/StepperController.h"
#include <atomic>

namespace aviator::platform::motor {

inline constexpr uint8_t INVALID_MOTOR_PIN = 0xFF;

/**
 * @brief Configuration parameters for StepperHandActuator.
 */
struct StepperActuatorConfig {
    int fullStepsPerRevolution{1600};
    double normalSpeedDegreesPerSecond{360.0};
    double normalAccelerationDegreesPerSecondSquared{1000.0};
    double jumpSpeedDegreesPerSecond{720.0};
    uint8_t enablePin{INVALID_MOTOR_PIN};
    bool enablePinActiveLow{true};
    bool autoDisableWhenIdle{true};
};

/**
 * @brief Concrete adapter implementing the domain HandActuator port for a stepper motor.
 * 
 * Provides thread-safe, lock-free command dispatch from domain tasks into the centralized
 * motor execution runner. Angle and homing updates are accepted asynchronously without blocking.
 */
class StepperHandActuator : public aviator::domain::HandActuator {
public:
    static constexpr uint8_t INVALID_PIN = INVALID_MOTOR_PIN;
    using ActuatorConfig = StepperActuatorConfig;

    enum class State {
        IDLE,
        MOVING,
        HOMING_IN_PROGRESS,
        HOMING_FAILED
    };

    StepperHandActuator(
        StepperController& stepperController,
        HomingStrategy& homingStrategy,
        const StepperActuatorConfig& config = StepperActuatorConfig{},
        logging::Logger* logger = nullptr);

    ~StepperHandActuator() override = default;

    // --- aviator::domain::HandActuator Port Implementation ---
    void setTargetAngle(double degrees) override;
    double getTargetAngle() const override;

    void setCurrentAngle(double degrees) override;
    double getCurrentAngle() const override;

    bool home() override;
    bool isHomed() const override;
    bool isBusy() const override;
    void stop() override;

    // --- Platform & Motor Runner Methods ---
    /**
     * @brief Steps the motor controller and processes pending asynchronous commands.
     * Called exclusively by the centralized motor runner task loop.
     * @return True if the actuator is actively moving or homing, false if idle at target.
     */
    bool update();

    void enable();
    void disable();

    bool isHomingFailed() const;
    bool isHoming() const;
    State getState() const;

    void setSpeed(double degreesPerSecond);
    void setAcceleration(double degreesPerSecondSquared);
    double getSpeed() const;
    double getAcceleration() const;

    long degreesToSteps(double degrees) const;
    double stepsToDegrees(long steps) const;

    const StepperActuatorConfig& getConfig() const { return _config; }
    StepperController& getStepperController() { return _stepperController; }
    HomingStrategy& getHomingStrategy() { return _homingStrategy; }

private:
    StepperController& _stepperController;
    HomingStrategy& _homingStrategy;
    StepperActuatorConfig _config;
    logging::Logger* _logger;

    // Synchronized state shared between domain threads and motor runner task
    std::atomic<double> _targetAngle{0.0};
    std::atomic<double> _currentAngle{0.0};

    std::atomic<double> _pendingTargetAngle{0.0};
    std::atomic<bool> _hasPendingTarget{false};

    std::atomic<double> _pendingResetAngle{0.0};
    std::atomic<bool> _hasPendingReset{false};

    std::atomic<bool> _hasPendingHome{false};
    std::atomic<bool> _hasPendingStop{false};

    std::atomic<bool> _isHomed{false};
    std::atomic<bool> _isHomingFailed{false};

    State _state{State::IDLE};
    double _configuredSpeedDegreesPerSecond;
    double _configuredAccelerationDegreesPerSecondSquared;
};

} // namespace aviator::platform::motor
