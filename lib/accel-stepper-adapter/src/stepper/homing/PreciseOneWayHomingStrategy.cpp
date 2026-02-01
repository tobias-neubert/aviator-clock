#include "stepper/homing/PreciseOneWayHomingStrategy.h"

#ifndef BUILTIN_LED
#define BUILTIN_LED 2 // Define BUILTIN_LED for potential future debugging use.
#endif

namespace stepper
{
    namespace homing
    {
        static const char *TAG = "PreciseOneWayHoming";

        PreciseOneWayHomingStrategy::PreciseOneWayHomingStrategy(
            soc::api::Logger &logger,
            stepper::api::IStepperController &stepperController,
            soc::api::IDigitalInput &limitSwitch,
            const Config &config) : _logger(logger),
                                     _stepperController(stepperController),
                                     _limitSwitch(limitSwitch),
                                     _config(config),
                                     _phase(Phase::NOT_STARTED),
                                     _result(stepper::api::HomingResult::NOT_YET_STARTED)
        {
        }

        void PreciseOneWayHomingStrategy::resetStrategy()
        {
            enterPhase(Phase::NOT_STARTED);
            _result = stepper::api::HomingResult::NOT_YET_STARTED;
        }

        bool PreciseOneWayHomingStrategy::beginHoming()
        {
            resetStrategy();
            _logger.i(TAG, "Beginning homing procedure.");

            _stepperController.setAcceleration(_config.homingAcceleration);
            _stepperController.setMaxSpeed(_config.homingSpeed);

            if (_limitSwitch.isActive())
            {
                _logger.i(TAG, "Switch is already active. Backing off.");
                // Move away from the switch
                _stepperController.move(-_config.moveDirection * _config.backOffDistance);
                enterPhase(Phase::MOVING_OFF_SWITCH);
            }
            else
            {
                _logger.i(TAG, "Switch is not active. Seeking switch.");
                // Start the main seek
                _stepperController.move(_config.moveDirection * _config.maxHomingTravel);
                enterPhase(Phase::SEEKING_SWITCH);
            }
            return true;
        }

        stepper::api::HomingResult PreciseOneWayHomingStrategy::updateHoming()
        {
            if (_phase == Phase::DONE)
            {
                return _result;
            }
            
            // Run the motor controller. If it returns false, the move is complete.
            bool isStillMoving = _stepperController.run();
            _logger.d(TAG, "Phase: %d, isStillMoving: %s, distanceToGo: %ld", static_cast<int>(_phase), isStillMoving ? "true" : "false", _stepperController.distanceToGo());

            switch (_phase)
            {
            case Phase::MOVING_OFF_SWITCH:
                if (!isStillMoving) // Finished moving away from the switch
                {
                    if (_limitSwitch.isActive()) {
                        _logger.e(TAG, "Failed to move off switch. Is back-off distance too small?");
                        _result = stepper::api::HomingResult::FAILURE_MOTOR_ERROR;
                        enterPhase(Phase::DONE);
                    } else {
                        _logger.i(TAG, "Successfully moved off switch. Starting seek.");
                        _stepperController.move(_config.moveDirection * _config.maxHomingTravel);
                        enterPhase(Phase::SEEKING_SWITCH);
                    }
                }
                break;

            case Phase::SEEKING_SWITCH:
                if (_limitSwitch.isActive())
                {
                    _logger.i(TAG, "Limit switch detected! Decelerating."); // New INFO log
                    _stepperController.stop(); // Issues a stop command, will decelerate
                    enterPhase(Phase::DECELERATING);
                }
                else if (!isStillMoving) // Reached max travel distance
                {
                    _logger.w(TAG, "Homing timed out. Did not find switch within max travel distance.");
                    _result = stepper::api::HomingResult::FAILURE_TIMEOUT;
                    enterPhase(Phase::DONE);
                }
                else
                {
                    _logger.d(TAG, "Seeking... Switch is not active.");
                }
                break;

            case Phase::DECELERATING:
                if (!isStillMoving) // Motor has come to a complete stop
                {
                    _logger.i(TAG, "Homing successful. Setting current position to 0.");
                    _stepperController.setCurrentPosition(0);
                    _result = stepper::api::HomingResult::SUCCESS;
                    enterPhase(Phase::DONE);
                }
                break;

            case Phase::NOT_STARTED:
            case Phase::DONE:
                // Do nothing in these states
                break;
            }

            return _result;
        }

        stepper::api::HomingResult PreciseOneWayHomingStrategy::getHomingResult() const
        {
            return _result;
        }

        void PreciseOneWayHomingStrategy::cancelHoming()
        {
            _logger.w(TAG, "Homing cancelled.");
            _stepperController.stop();
            _result = stepper::api::HomingResult::CANCELLED;
            enterPhase(Phase::DONE);
        }

        void PreciseOneWayHomingStrategy::enterPhase(Phase newPhase)
        {
            _phase = newPhase;
            if (_phase != Phase::DONE)
            {
                _result = stepper::api::HomingResult::IN_PROGRESS;
            }
        }
    }
}