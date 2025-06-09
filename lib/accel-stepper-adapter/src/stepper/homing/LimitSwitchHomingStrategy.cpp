#include <stepper/homing/LimitSwitchHomingStrategy.h>
#include <cmath>

namespace stepper
{
    namespace homing
    {
        LimitSwitchHomingStrategy::LimitSwitchHomingStrategy(
            stepper::api::IStepperController &stepperController,
            soc::api::IDigitalInput &limitSwitch,
            const Config &config,
            soc::api::ILogger& logger)
            : _stepperController(stepperController),
              _limitSwitch(limitSwitch),
              _config(config),
              _logger(logger),
              _currentPhase(HomingPhase::NOT_STARTED),
              _currentHomingResult(stepper::api::HomingResult::NOT_YET_STARTED)
        {
            if (_config.maxHomingTravelSteps <= 0)
            {
                // Or throw an exception, or handle this error appropriately
                // For simplicity, we'll assume valid config for now or let it misbehave.
                // In a real scenario, validate config.maxHomingTravelSteps.
            }
        }

        void LimitSwitchHomingStrategy::resetStrategy()
        {
            _currentPhase = HomingPhase::NOT_STARTED;
            _currentHomingResult = stepper::api::HomingResult::NOT_YET_STARTED;
        }

        bool LimitSwitchHomingStrategy::beginHoming()
        {
            resetStrategy();

            _stepperController.setMaxSpeed(static_cast<float>(_config.homingSpeedStepsPerSec));
            _stepperController.setAcceleration(static_cast<float>(_config.homingAccelerationStepsPerSecSq));

            // Calculate a target position far away in the homing direction
            // this does not actually move the motor
            long targetRelativeSteps = _config.moveDirectionSign * _config.maxHomingTravelSteps;
            _stepperController.move(targetRelativeSteps);

            if (_stepperController.distanceToGo() != 0)
            {
                _currentPhase = HomingPhase::MOVING_TOWARDS_SWITCH;
                _currentHomingResult = stepper::api::HomingResult::IN_PROGRESS;
                return true;
            }
            else
            {
                // distanceToGo() == 0, meaning: Cannot move for whatever reason
                // For robustness, check switch state here too.
                if (_limitSwitch.isActive())
                {
                    _stepperController.setCurrentPosition(0);
                    _currentPhase = HomingPhase::DECELERATING_ON_SWITCH;
                    _currentHomingResult = stepper::api::HomingResult::SUCCESS;
                    return true;
                }

                _currentHomingResult = stepper::api::HomingResult::INITIATION_FAILURE;
                return false;
            }
        }

        stepper::api::HomingResult LimitSwitchHomingStrategy::updateHoming()
        {
            switch (_currentPhase)
            {
            case HomingPhase::NOT_STARTED:
                // Should not be called in this state usually, but handle defensively
                _currentHomingResult = stepper::api::HomingResult::INITIATION_FAILURE;
                break;
            case HomingPhase::MOVING_TOWARDS_SWITCH:
                _stepperController.run();

                if (_limitSwitch.isActive())
                {
                    _logger.info("Limit switch is active");
                    _stepperController.stop();
                    _currentPhase = HomingPhase::DECELERATING_ON_SWITCH;
                    _currentHomingResult = stepper::api::HomingResult::IN_PROGRESS;
                }
                else if (_stepperController.distanceToGo() == 0)
                {
                    _currentPhase = HomingPhase::TIMED_OUT;
                    _currentHomingResult = stepper::api::HomingResult::FAILURE_TIMEOUT;
                }
                break;
            case HomingPhase::DECELERATING_ON_SWITCH:
                // Keep calling run() until AccelStepper has finished decelerating and stopped.
                // AccelStepper::run() returns false when it is stopped (at target and speed is zero).
                if (!_stepperController.run())
                {
                    _logger.info("LimitSwitchHomingStrategy: Homing completed.");
                    _stepperController.setCurrentPosition(0);
                    _currentHomingResult = stepper::api::HomingResult::SUCCESS;
                }
                else
                {
                    _currentHomingResult = stepper::api::HomingResult::IN_PROGRESS;
                }
                break;
            case HomingPhase::TIMED_OUT:
                break;
            }
            return _currentHomingResult;
        }

        stepper::api::HomingResult LimitSwitchHomingStrategy::getHomingResult() const
        {
            return _currentHomingResult;
        }

        void LimitSwitchHomingStrategy::cancelHoming()
        {
            // This is a hard cancel, try to stop the motor immediately.
            // The AccelStepperMotor class might also call its own stop() or _stepperController.stop().
            // It's good for the strategy to also attempt to halt its own commanded motion.
            if (_currentPhase == HomingPhase::MOVING_TOWARDS_SWITCH || _currentPhase == HomingPhase::DECELERATING_ON_SWITCH)
            {
                _stepperController.stop();
                // To ensure it stops, one might need to keep calling run in a loop here,
                // but cancelHoming is usually for immediate effect. The AccelStepperMotor's
                // state machine should handle further calls to run if it transitions to MOVING.
            }
            _currentPhase = HomingPhase::NOT_STARTED;
            _currentHomingResult = stepper::api::HomingResult::CANCELLED;
        }
    } // namespace homing
} // namespace stepper