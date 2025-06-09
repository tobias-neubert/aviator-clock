#include <aviator-clock/ClockHand.h>

namespace aviator_clock
{
    ClockHand::ClockHand(HandType type,
                         soc::api::ITime &timeProvider,
                         std::unique_ptr<IStepperMotor> stepperMotor,
                         soc::api::ILogger &logger,
                         double totalAngleForDial,
                         double dialStartOffsetDegrees,
                         double motorSpeedDps,
                         double motorAccelerationDps2)
        : _type(type),
          _timeProvider(timeProvider),
          // move transfers ownership of the pointer
          // whoever created the stepperMotor, now
          // it belongs to this hand. If the hand is destroyed
          // the stepperMotor will be as well.
          _stepperMotor(std::move(stepperMotor)),
          _logger(logger),
          _lastUnitProcessed(-1),
          _isPositionInitialized(false),
          _totalAngleForDial(totalAngleForDial),
          _dialStartOffsetDegrees(dialStartOffsetDegrees),
          _motorSpeedDps(motorSpeedDps),
          _motorAccelerationDps2(motorAccelerationDps2),
          _homingFailed(false),
          _operationStarted(false)
    {
        if (_stepperMotor == nullptr)
        {
            _logger.error("ERROR: Null stepper motor provided to ClockHand");
        }

        if (_totalAngleForDial <= 0.0 || _totalAngleForDial > 360.0)
        {
            _logger.warn("Warning: Invalid totalAngleForDial (%.2f). Defaulting to 360.0", totalAngleForDial);
            _totalAngleForDial = 360.0;
        }

        // Calculate degrees per unit based on the hand type
        int unitsOnDial = 0;
        switch (_type)
        {
        case HandType::HOUR:
            unitsOnDial = 12;
            break;
        case HandType::MINUTE:
        case HandType::SECOND:
            unitsOnDial = 60;
            break;
        }

        if (unitsOnDial <= 0)
        {
            _logger.error("Units on dial must be greater than 0 but is %.2f", unitsOnDial);
        }
        else
        {
            _degreesPerUnit = _totalAngleForDial / static_cast<double>(unitsOnDial);
        }
    }

    // --- ISocComponent Methods ---
    void ClockHand::setup()
    {
        if (!_stepperMotor)
            return;

        _stepperMotor->setSpeed(_motorSpeedDps);
        _stepperMotor->setAcceleration(_motorAccelerationDps2);
        _logger.info("ClockHand::setup() - Motor configured. Speed=%.2f, Accel=%.2f", _motorSpeedDps, _motorAccelerationDps2);

        _stepperMotor->enable();
        if (!_stepperMotor->home())
        {
            _logger.error("ClockHand: Stepper homing failed!");
        }
        else
        {
            _logger.info("ClockHand: Stepper homing started.");
        }
        _isPositionInitialized = false;
        _lastUnitProcessed = -1;
    }

    void ClockHand::advanceState(unsigned long currentTimeMs)
    {
        if (!_stepperMotor)
            return;

        _stepperMotor->update();

        // === STATE A: Homing is Required ===
        if (_stepperMotor->needsHoming())
        {
            if (_stepperMotor->isHomingFailed() && !_homingFailed)
            {
                _logger.error("ClockHand: Homing has failed! Please check motor and limit switch. System halted.");
                _homingFailed = true;
            }

            // While homing (or if failed), do not proceed to clock logic.
            return;
        }

        // === STATE B: Homing is Complete and Successful ===
        // We can only proceed if the motor isn't busy with a final homing move or a previous tick.
        if (_stepperMotor->isBusy())
        {
            return;
        }

        if (!_operationStarted)
        {
            _logger.info("ClockHand: Homing successful! Motor is at home position.");
            _logger.info("ClockHand: Initializing position...");
            _operationStarted = true;

            soc::api::ITime::TimeComponents currentTime;
            if (_timeProvider.asTimeComponents(currentTime))
            {
                int currentUnit = 0;
                switch (_type)
                {
                case HandType::HOUR:
                    currentUnit = currentTime.hours % 12;
                    break;
                case HandType::MINUTE:
                    currentUnit = currentTime.minutes;
                    break;
                case HandType::SECOND:
                    currentUnit = currentTime.seconds;
                    break;
                }
                moveToUnit(currentUnit);
                _lastUnitProcessed = currentUnit;
            }
        }
        // === STATE C: Normal Ticking Operation ===
        else
        {
            soc::api::ITime::TimeComponents currentTime;
            if (!_timeProvider.asTimeComponents(currentTime))
            {
                _logger.error("timeProvider did not return true. So I am doing nothing.");
                return;
            }

            int currentUnit = 0;
            switch (_type)
            {
            case HandType::HOUR:
                currentUnit = currentTime.hours % 12;
                break;
            case HandType::MINUTE:
                currentUnit = currentTime.minutes;
                break;
            case HandType::SECOND:
                currentUnit = currentTime.seconds;
                break;
            }

            if (currentUnit != _lastUnitProcessed)
            {
                moveToUnit(currentUnit);
                _lastUnitProcessed = currentUnit;
            }
        }
    }

    void ClockHand::render()
    {
    }

    void ClockHand::teardown()
    {
        if (!_stepperMotor)
            return;

        if (_stepperMotor->isBusy())
        {
            _stepperMotor->stop();
        }
        _stepperMotor->disable();
    }

    // --- Private Methods ---
    void ClockHand::moveToUnit(int unit)
    {
        if (!_stepperMotor)
        {
            return;
        }

        // Handle the 12-o'clock case for hours, which is often represented as 0
        int unitForCalc = unit;
        if (_type == HandType::HOUR && unit == 0)
        { // 12 AM or 12 PM
            unitForCalc = 12;
        }

        double targetAngle = _dialStartOffsetDegrees + (static_cast<double>(unitForCalc) * _degreesPerUnit);

        _logger.debug("ClockHand: Moving to unit %d (Angle: %.2f)", unit, targetAngle);
        if (!_stepperMotor->moveToAbsolute(targetAngle))
        {
            _logger.error("stepper motor did not move to %.2f", targetAngle);
        }
    }
} // namespace aviator_clock