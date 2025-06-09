#include <stepper/accel/AccelStepperMotor.h>
#include <stepper/api/StepperMotorState.h>
#include <cmath>
#include <stdexcept>

// A common invalid pin marker (ensure it's defined if not in a global header)
#ifndef INVALID_PIN
#define INVALID_PIN 0xFF
#endif

using stepper::api::StepperMotorState;

namespace stepper
{
    namespace accel
    {
        AccelStepperMotor::AccelStepperMotor(
            stepper::api::IStepperController &stepperController,
            int fullStepsPerRevolution,
            stepper::api::IHomingStrategy &homingStrategy,
            soc::api::ILogger& logger,
            uint8_t enablePin,
            bool enablePinActiveLow) : _stepperController(stepperController),
                                       _fullStepsPerRevolution(fullStepsPerRevolution),
                                       _homingStrategy(homingStrategy),
                                       _logger(logger),
                                       _enablePin(enablePin),
                                       _isHomed(false),                                     // Motor starts as not homed
                                       _currentSetSpeedDps(100.0),                          // Default speed
                                       _currentSetAccelerationDps2(100.0),                  // Default acceleration
                                       _currentState(stepper::api::StepperMotorState::IDLE) // Initialize state
        {
            homingStrategy.resetStrategy();

            if (_enablePin != INVALID_PIN)
            {
                _stepperController.setEnablePin(_enablePin);
                // The third argument to setPinsInverted is 'enablePinsInverted'.
                // If enablePinActiveLow is true (meaning LOW enables), then enablePinsInverted should be true.
                _stepperController.setPinsInverted(false, false, enablePinActiveLow);
                _stepperController.disableOutputs();
            }

            _stepperController.setMaxSpeed(degreesToSteps(_currentSetSpeedDps));
            _stepperController.setAcceleration(degreesToSteps(_currentSetAccelerationDps2));
        }

        long AccelStepperMotor::degreesToSteps(double degrees) const
        {
            return static_cast<long>((degrees / 360.0) * _fullStepsPerRevolution);
        }

        double AccelStepperMotor::stepsToDegrees(long steps) const
        {
            return (static_cast<double>(steps) / _fullStepsPerRevolution) * 360.0;
        }

        bool AccelStepperMotor::home()
        {
            if (_currentState == StepperMotorState::MOVING ||
                _currentState == StepperMotorState::HOMING_IN_PROGRESS)
            {
                return false;
            }

            _homingStrategy.resetStrategy();
            if (!_homingStrategy.beginHoming())
            {
                _currentState = StepperMotorState::HOMING_FAILED;
                return false;
            }

            _currentState = StepperMotorState::HOMING_IN_PROGRESS;
            _isHomed = false;

            // For instant strategies, check status immediately
            // e.g. NotHomingStrategy
            stepper::api::HomingResult strategyStatus = _homingStrategy.getHomingResult();
            if (strategyStatus == stepper::api::HomingResult::SUCCESS)
            {
                _stepperController.setCurrentPosition(0);
                _isHomed = true;
                _currentState = StepperMotorState::IDLE;
            }

            return true;
        }

        StepperMotorState AccelStepperMotor::getState() const 
        {
            return _currentState;
        }

        bool AccelStepperMotor::isHomingFailed() const
        {
            return _currentState == StepperMotorState::HOMING_FAILED;
        }

        bool AccelStepperMotor::isHoming() const
        {
            return _currentState == StepperMotorState::HOMING_IN_PROGRESS;
        }

        bool AccelStepperMotor::needsHoming() const
        {
            return !_isHomed;
        }

        bool AccelStepperMotor::moveToAbsolute(double degreesAbsolute)
        {
            if (needsHoming())
            {
                return false;
            }

            if (_currentState != StepperMotorState::IDLE)
            {
                return false;
            }

            if (_enablePin != INVALID_PIN)
            {
                enable();
            }

            _stepperController.setMaxSpeed(degreesToSteps(_currentSetSpeedDps));
            _stepperController.setAcceleration(degreesToSteps(_currentSetAccelerationDps2));
            _stepperController.moveTo(degreesToSteps(degreesAbsolute));

            if (_stepperController.distanceToGo() != 0)
            {
                _currentState = StepperMotorState::MOVING;
            }
            else
            {
                _currentState = StepperMotorState::IDLE;
            }

            return true;
        }

        bool AccelStepperMotor::rotateRelative(double degreesRelative)
        {
            if (_currentState != StepperMotorState::IDLE)
            {
                return false;
            }

            if (_enablePin != INVALID_PIN)
            {
                enable();
            }

            _stepperController.setMaxSpeed(degreesToSteps(_currentSetSpeedDps));
            _stepperController.setAcceleration(degreesToSteps(_currentSetAccelerationDps2));
            _stepperController.move(degreesToSteps(degreesRelative));

            if (_stepperController.distanceToGo() != 0)
            {
                _currentState = StepperMotorState::MOVING;
            }
            else
            {
                _currentState = StepperMotorState::IDLE;
            }

            return true;
        }

        void AccelStepperMotor::setSpeed(double degreesPerSecond)
        {
            if (degreesPerSecond <= 0)
            {
                _currentSetSpeedDps = 1.0;
            }
            else
            {
                _currentSetSpeedDps = degreesPerSecond;
                _logger.info("Set speed to %f", degreesPerSecond);
            }

            _stepperController.setMaxSpeed(degreesToSteps(_currentSetSpeedDps));
        }

        void AccelStepperMotor::setAcceleration(double degreesPerSecondSquared)
        {
            if (degreesPerSecondSquared <= 0)
            {
                _currentSetAccelerationDps2 = 1.0;
            }
            else
            {
                _currentSetAccelerationDps2 = degreesPerSecondSquared;
            }
            _stepperController.setAcceleration(degreesToSteps(_currentSetAccelerationDps2));
        }

        double AccelStepperMotor::getCurrentPositionDegrees() const
        {
            if (!_isHomed)
            {
                throw std::runtime_error("Motor not homed, position unknown.");
            }
            return stepsToDegrees(_stepperController.getCurrentPosition());
        }

        void AccelStepperMotor::enable()
        {
            if (_enablePin != INVALID_PIN)
            {
                _stepperController.enableOutputs();
            }
        }

        void AccelStepperMotor::disable()
        {
            if (_enablePin != INVALID_PIN)
            {
                _stepperController.disableOutputs();
            }
        }

        //
        // --- Non-blocking state machine methods ---
        //
        void AccelStepperMotor::update()
        {
            if (_currentState == StepperMotorState::IDLE)
            {
                return;
            }

            if (_currentState == StepperMotorState::MOVING)
            {
                _stepperController.run();

                // Check if the motor has reached its target destination.
                if (_stepperController.distanceToGo() == 0)
                {
                    // At this point, the motor is at the target.
                    // Call run() one more time. If AccelStepper was in its final step of deceleration
                    // or just arriving, this ensures its internal speed state is also finalized to zero,
                    // and it should then definitively return false if it's truly stopped at the target.
                    // This helps ensure that AccelStepper itself acknowledges completion.
                    if (!_stepperController.run())
                    {
                        _currentState = StepperMotorState::IDLE;
                        // You can add a log here for debugging:
                        // Serial.println("Adapter: Move complete. State -> IDLE.");
                    }
                    else
                    {
                        // This case is less common if distanceToGo() is already 0.
                        // It might mean AccelStepper still thinks it has a micro-step of speed to shed.
                        // Keep calling run() via the update loop. It should resolve in the next cycle(s).
                        // Serial.println("Adapter: Dist=0 but run() still true. Waiting for speed to zero.");
                    }
                }
            }
            else if (_currentState == StepperMotorState::HOMING_IN_PROGRESS)
            {
                stepper::api::HomingResult homingStatus = _homingStrategy.updateHoming();

                if (homingStatus == stepper::api::HomingResult::SUCCESS)
                {
                    _stepperController.setCurrentPosition(0);
                    _isHomed = true;
                    _currentState = StepperMotorState::IDLE;

                    // Restore operational parameters on the controller
                    _logger.info(
                        "AccelStepperMotor: Homing succeeded, setting speed and acceleration to %f and %f", 
                        _currentSetSpeedDps, 
                        _currentSetAccelerationDps2);
                    _stepperController.setMaxSpeed(degreesToSteps(_currentSetSpeedDps));
                    _stepperController.setAcceleration(degreesToSteps(_currentSetAccelerationDps2));
                }
                else if (homingStatus != stepper::api::HomingResult::IN_PROGRESS)
                {
                    _currentState = StepperMotorState::HOMING_FAILED;
                    _stepperController.stop();
                }
            }
        }

        bool AccelStepperMotor::isBusy() const
        {
            return _currentState != StepperMotorState::IDLE;
        }

        void AccelStepperMotor::stop()
        {
            if (_currentState == StepperMotorState::IDLE && _stepperController.distanceToGo() == 0)
            {
                return; // Already idle and at target, nothing to stop
            }

            _stepperController.stop(); // Tell AccelStepper to decelerate to the current position.
                                       // This sets a new target (current position) and AccelStepper
                                       // will decelerate when run() is called.

            // If it was moving or had a non-zero distance to go, it's now effectively in a MOVING state
            // (decelerating to the stop point). If it was already idle but not at its target
            // (e.g., stop() called twice), this ensures it tries to go to that stopped target.
            if (_stepperController.distanceToGo() != 0)
            {
                _currentState = StepperMotorState::MOVING;
            }
            else
            {
                // If stop() was called but distanceToGo was already 0 (e.g. already stopped at target)
                _currentState = StepperMotorState::IDLE;
            }
        }
    }
}