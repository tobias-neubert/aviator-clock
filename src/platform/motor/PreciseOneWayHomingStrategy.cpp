#include "platform/motor/PreciseOneWayHomingStrategy.h"

namespace aviator::platform::motor {

static const char* TAG = "PreciseOneWayHoming";

PreciseOneWayHomingStrategy::PreciseOneWayHomingStrategy(
    StepperController& stepperController,
    soc::api::IDigitalInput& limitSwitch,
    const Config& config,
    soc::api::Logger* logger)
    : _stepperController(stepperController),
      _limitSwitch(limitSwitch),
      _config(config),
      _logger(logger),
      _phase(Phase::NOT_STARTED),
      _result(HomingResult::NOT_YET_STARTED) {}

void PreciseOneWayHomingStrategy::resetStrategy() {
    enterPhase(Phase::NOT_STARTED);
    _result = HomingResult::NOT_YET_STARTED;
}

bool PreciseOneWayHomingStrategy::beginHoming() {
    resetStrategy();
    if (_logger != nullptr) {
        _logger->i(TAG, "Beginning homing procedure.");
    }

    _stepperController.setAcceleration(_config.homingAcceleration);
    _stepperController.setMaxSpeed(_config.homingSpeed);

    if (_limitSwitch.isActive()) {
        if (_logger != nullptr) {
            _logger->i(TAG, "Sensor is already active. Backing off.");
        }
        _stepperController.move(-_config.moveDirection * _config.backOffDistance);
        enterPhase(Phase::MOVING_OFF_SWITCH);
    } else {
        if (_logger != nullptr) {
            _logger->i(TAG, "Sensor is inactive. Seeking sensor trigger.");
        }
        _stepperController.move(_config.moveDirection * _config.maxHomingTravel);
        enterPhase(Phase::SEEKING_SWITCH);
    }
    return true;
}

HomingResult PreciseOneWayHomingStrategy::updateHoming() {
    if (_phase == Phase::DONE) {
        return _result;
    }

    const bool isStillMoving = _stepperController.run();

    switch (_phase) {
    case Phase::MOVING_OFF_SWITCH:
        if (!isStillMoving) {
            if (_limitSwitch.isActive()) {
                if (_logger != nullptr) {
                    _logger->e(TAG, "Failed to back off sensor. Sensor remains active.");
                }
                _result = HomingResult::FAILURE_MOTOR_ERROR;
                enterPhase(Phase::DONE);
            } else {
                if (_logger != nullptr) {
                    _logger->i(TAG, "Backed off sensor successfully. Seeking sensor trigger.");
                }
                _stepperController.move(_config.moveDirection * _config.maxHomingTravel);
                enterPhase(Phase::SEEKING_SWITCH);
            }
        }
        break;

    case Phase::SEEKING_SWITCH:
        if (_limitSwitch.isActive()) {
            if (_logger != nullptr) {
                _logger->i(TAG, "Sensor detected. Stopping to decelerate to zero point.");
            }
            _stepperController.stop();
            enterPhase(Phase::DECELERATING);
        } else if (!isStillMoving) {
            if (_logger != nullptr) {
                _logger->w(TAG, "Homing timed out. Maximum travel distance reached without detecting sensor.");
            }
            _result = HomingResult::FAILURE_TIMEOUT;
            enterPhase(Phase::DONE);
        }
        break;

    case Phase::DECELERATING:
        if (!isStillMoving) {
            if (_logger != nullptr) {
                _logger->i(TAG, "Deceleration complete. Zero datum established.");
            }
            _stepperController.setCurrentPosition(0);
            _result = HomingResult::SUCCESS;
            enterPhase(Phase::DONE);
        }
        break;

    case Phase::NOT_STARTED:
    case Phase::DONE:
        break;
    }

    return _result;
}

HomingResult PreciseOneWayHomingStrategy::getHomingResult() const {
    return _result;
}

void PreciseOneWayHomingStrategy::cancelHoming() {
    if (_logger != nullptr) {
        _logger->w(TAG, "Homing cancelled.");
    }
    _stepperController.stop();
    _result = HomingResult::CANCELLED;
    enterPhase(Phase::DONE);
}

void PreciseOneWayHomingStrategy::enterPhase(Phase newPhase) {
    _phase = newPhase;
    if (_phase != Phase::DONE) {
        _result = HomingResult::IN_PROGRESS;
    }
}

} // namespace aviator::platform::motor
