#include "platform/motor/StepperHandActuator.h"
#include <cmath>

namespace aviator::platform::motor {

static const char* TAG = "StepperHandActuator";

StepperHandActuator::StepperHandActuator(
    StepperController& stepperController,
    HomingStrategy& homingStrategy,
    const ActuatorConfig& config,
    soc::api::Logger* logger)
    : _stepperController(stepperController),
      _homingStrategy(homingStrategy),
      _config(config),
      _logger(logger),
      _configuredSpeedDegreesPerSecond(config.normalSpeedDegreesPerSecond),
      _configuredAccelerationDegreesPerSecondSquared(config.normalAccelerationDegreesPerSecondSquared) {

    _homingStrategy.resetStrategy();

    if (_config.enablePin != INVALID_PIN) {
        _stepperController.setEnablePin(_config.enablePin);
        _stepperController.setPinsInverted(false, false, _config.enablePinActiveLow);
        _stepperController.disableOutputs();
    }

    _stepperController.setMaxSpeed(degreesToSteps(_configuredSpeedDegreesPerSecond));
    _stepperController.setAcceleration(degreesToSteps(_configuredAccelerationDegreesPerSecondSquared));
}

void StepperHandActuator::setTargetAngle(double degrees) {
    if (_logger != nullptr) {
        _logger->d(TAG, "setTargetAngle commanded: %.2f degrees", degrees);
    }
    _targetAngle.store(degrees);
    _pendingTargetAngle.store(degrees);
    _hasPendingTarget.store(true);
}

double StepperHandActuator::getTargetAngle() const {
    return _targetAngle.load();
}

void StepperHandActuator::setCurrentAngle(double degrees) {
    _currentAngle.store(degrees);
    _pendingResetAngle.store(degrees);
    _hasPendingReset.store(true);
}

double StepperHandActuator::getCurrentAngle() const {
    return _currentAngle.load();
}

bool StepperHandActuator::home() {
    if (_state == State::HOMING_IN_PROGRESS || _hasPendingHome.load()) {
        return false;
    }
    _hasPendingHome.store(true);
    _isHomingFailed.store(false);
    return true;
}

bool StepperHandActuator::isHomed() const {
    return _isHomed.load();
}

bool StepperHandActuator::isBusy() const {
    return _state == State::MOVING ||
           _state == State::HOMING_IN_PROGRESS ||
           _hasPendingHome.load() ||
           _hasPendingTarget.load();
}

void StepperHandActuator::stop() {
    _hasPendingStop.store(true);
}

bool StepperHandActuator::update() {
    // 1. Process asynchronous stop command
    if (_hasPendingStop.exchange(false)) {
        if (_state == State::HOMING_IN_PROGRESS) {
            _homingStrategy.cancelHoming();
        }
        _stepperController.stop();
        _hasPendingTarget.store(false);
        _hasPendingHome.store(false);

        if (_stepperController.distanceToGo() == 0) {
            _state = State::IDLE;
            if (_config.autoDisableWhenIdle) {
                disable();
            }
        } else {
            _state = State::MOVING;
        }
    }

    // 2. Process asynchronous homing command
    if (_hasPendingHome.exchange(false)) {
        enable();
        _isHomed.store(false);
        _isHomingFailed.store(false);
        _homingStrategy.resetStrategy();

        if (_homingStrategy.beginHoming()) {
            _state = State::HOMING_IN_PROGRESS;
            // Check for immediate completion (e.g. instant homing strategy in tests)
            if (_homingStrategy.getHomingResult() == HomingResult::SUCCESS) {
                _stepperController.setCurrentPosition(0);
                _isHomed.store(true);
                _state = State::IDLE;
                _currentAngle.store(0.0);
                _targetAngle.store(0.0);
                if (_config.autoDisableWhenIdle) {
                    disable();
                }
            }
        } else {
            _state = State::HOMING_FAILED;
            _isHomingFailed.store(true);
        }
        return isBusy();
    }

    // 3. Step in-progress homing
    if (_state == State::HOMING_IN_PROGRESS) {
        const HomingResult homingResult = _homingStrategy.updateHoming();
        if (homingResult == HomingResult::SUCCESS) {
            _stepperController.setCurrentPosition(0);
            _isHomed.store(true);
            _state = State::IDLE;
            _currentAngle.store(0.0);
            _targetAngle.store(0.0);
            _stepperController.setMaxSpeed(degreesToSteps(_configuredSpeedDegreesPerSecond));
            _stepperController.setAcceleration(degreesToSteps(_configuredAccelerationDegreesPerSecondSquared));
            if (_config.autoDisableWhenIdle) {
                disable();
            }
        } else if (homingResult != HomingResult::IN_PROGRESS) {
            _state = State::HOMING_FAILED;
            _isHomingFailed.store(true);
            _stepperController.stop();
        }
        return isBusy();
    }

    // 4. Process position reset override (used after 360-degree wrap-around jumps)
    if (_hasPendingReset.exchange(false)) {
        const double resetDegrees = _pendingResetAngle.load();
        _stepperController.setCurrentPosition(degreesToSteps(resetDegrees));
        _currentAngle.store(resetDegrees);
    }

    // 5. Process pending target angle movement
    if (_hasPendingTarget.exchange(false)) {
        if (_isHomed.load()) {
            const double targetDegrees = _pendingTargetAngle.load();
            enable();

            // When executing a wrap-around jump across 360 degrees, boost stepping speed
            // so the hand rapidly traverses the inactive dial gap.
            if (targetDegrees >= 360.0 && _config.jumpSpeedDegreesPerSecond > 0.0) {
                _stepperController.setMaxSpeed(degreesToSteps(_config.jumpSpeedDegreesPerSecond));
            } else {
                _stepperController.setMaxSpeed(degreesToSteps(_configuredSpeedDegreesPerSecond));
            }
            _stepperController.setAcceleration(degreesToSteps(_configuredAccelerationDegreesPerSecondSquared));
            _stepperController.moveTo(degreesToSteps(targetDegrees));

            if (_stepperController.distanceToGo() != 0) {
                _state = State::MOVING;
            } else {
                _state = State::IDLE;
                if (_config.autoDisableWhenIdle) {
                    disable();
                }
            }
        }
    }

    // 6. Step the active motor towards its target
    if (_state == State::MOVING) {
        _stepperController.run();
        _currentAngle.store(stepsToDegrees(_stepperController.getCurrentPosition()));

        if (_stepperController.distanceToGo() == 0) {
            if (!_stepperController.run()) {
                _state = State::IDLE;
                _stepperController.setMaxSpeed(degreesToSteps(_configuredSpeedDegreesPerSecond));
                if (_config.autoDisableWhenIdle) {
                    disable();
                }
            }
        }
    } else if (_state == State::IDLE) {
        _currentAngle.store(stepsToDegrees(_stepperController.getCurrentPosition()));
    }

    return isBusy();
}

void StepperHandActuator::enable() {
    if (_config.enablePin != INVALID_PIN) {
        _stepperController.enableOutputs();
    }
}

void StepperHandActuator::disable() {
    if (_config.enablePin != INVALID_PIN) {
        _stepperController.disableOutputs();
    }
}

bool StepperHandActuator::isHomingFailed() const {
    return _state == State::HOMING_FAILED || _isHomingFailed.load();
}

bool StepperHandActuator::isHoming() const {
    return _state == State::HOMING_IN_PROGRESS || _hasPendingHome.load();
}

StepperHandActuator::State StepperHandActuator::getState() const {
    return _state;
}

void StepperHandActuator::setSpeed(double degreesPerSecond) {
    if (degreesPerSecond <= 0.0) {
        _configuredSpeedDegreesPerSecond = 1.0;
    } else {
        _configuredSpeedDegreesPerSecond = degreesPerSecond;
    }
    _stepperController.setMaxSpeed(degreesToSteps(_configuredSpeedDegreesPerSecond));
}

void StepperHandActuator::setAcceleration(double degreesPerSecondSquared) {
    if (degreesPerSecondSquared <= 0.0) {
        _configuredAccelerationDegreesPerSecondSquared = 1.0;
    } else {
        _configuredAccelerationDegreesPerSecondSquared = degreesPerSecondSquared;
    }
    _stepperController.setAcceleration(degreesToSteps(_configuredAccelerationDegreesPerSecondSquared));
}

double StepperHandActuator::getSpeed() const {
    return _configuredSpeedDegreesPerSecond;
}

double StepperHandActuator::getAcceleration() const {
    return _configuredAccelerationDegreesPerSecondSquared;
}

long StepperHandActuator::degreesToSteps(double degrees) const {
    return static_cast<long>((degrees / 360.0) * _config.fullStepsPerRevolution);
}

double StepperHandActuator::stepsToDegrees(long steps) const {
    return (static_cast<double>(steps) / _config.fullStepsPerRevolution) * 360.0;
}

} // namespace aviator::platform::motor
