#include <aviator-clock/ClockHand.h>
#include <string>

static const char* TAG = "ClockHand";

namespace aviator_clock
{

  // --- Builder Implementation ---

  ClockHand::Builder::Builder(soc::api::ISoc &systemOnChip, std::unique_ptr<IStepperMotor> &&stepperMotor)
      : _systemOnChip(systemOnChip),
        _stepperMotor(std::move(stepperMotor))
  {
  }

  ClockHand::Builder &ClockHand::Builder::withHandType(HandType type)
  {
    _type = type;
    return *this;
  }

  ClockHand::Builder &ClockHand::Builder::withTotalAngleForDial(double angle)
  {
    _totalAngleForDial = angle;
    return *this;
  }

  ClockHand::Builder &ClockHand::Builder::withMotorSpeedDegreesPerSecond(double speed)
  {
    _motorSpeedDegreesPerSecond = speed;
    return *this;
  }

  ClockHand::Builder &ClockHand::Builder::withMotorAccelerationDegreesPerSecondSquared(double accel)
  {
    _motorAccelerationDegreesPerSecondSquared = accel;
    return *this;
  }

  std::unique_ptr<ClockHand> ClockHand::Builder::build()
  {
      return std::make_unique<ClockHand>(
          _type,
          _systemOnChip,
          std::move(_stepperMotor),
          _totalAngleForDial,
          _motorSpeedDegreesPerSecond,
          _motorAccelerationDegreesPerSecondSquared
      );
  }

  ClockHand::ClockHand(HandType type, soc::api::ISoc &systemOnChip,
                       std::unique_ptr<IStepperMotor> &&stepperMotor,
                       double totalAngleForDial,
                       double motorSpeedDegreesPerSecond, double motorAccelerationDegreesPerSecondSquared)
      : _type(type),
        _systemOnChip(systemOnChip),
        _logger(systemOnChip.getLogger()),
        _timeProvider(_systemOnChip.getTimeProvider()),
        _stepperMotor(std::move(stepperMotor)),
        _shouldStop(false),
        _totalAngleForDial(totalAngleForDial),
        _motorSpeedDegreesPerSecond(motorSpeedDegreesPerSecond),
        _motorAccelerationDegreesPerSecondSquared(motorAccelerationDegreesPerSecondSquared),
        _lastDisplayValue(-1)
  {
    if (_stepperMotor == nullptr)
    {
      _logger.e(TAG, "ERROR: Null stepper motor provided to ClockHand");
    }

    if (_totalAngleForDial <= 0.0 || _totalAngleForDial > 360.0)
    {
                 _logger.w(TAG, "Warning: Invalid totalAngleForDial (%.2f). Defaulting to 360.0", (double)_totalAngleForDial);
      _totalAngleForDial = 360.0;
    }

    int unitsOnDial = (_type == HandType::HOUR) ? 12 : 60;
    _degreesPerUnit = _totalAngleForDial / static_cast<double>(unitsOnDial);
  }

  ClockHand::~ClockHand() { teardown(); }

  void ClockHand::setup()
  {
    if (!_stepperMotor)
      return;
    _stepperMotor->setSpeed(_motorSpeedDegreesPerSecond);
    _stepperMotor->setAcceleration(_motorAccelerationDegreesPerSecondSquared);
    _logger.i(TAG, "ClockHand::setup() - Motor configured for %s hand.",
                 _type == HandType::SECOND ? "SECOND" : "OTHER");
  }

  void ClockHand::teardown()
  {
    _shouldStop = true; // Signal task to stop
    if (_task)
    {
      _task.reset();
    }
    if (!_stepperMotor)
      return;
    if (_stepperMotor->isBusy())
    {
      _stepperMotor->stop();
    }
    _stepperMotor->disable();
    _logger.i(TAG, "ClockHand::teardown() - Resources released.");
  }

  void ClockHand::start()
  {
    _logger.i(TAG, "ClockHand::start() - Starting task for %s hand.",
                 _type == HandType::SECOND ? "SECOND" : "OTHER");
    if (_task)
    {
      _logger.w(TAG, "ClockHand::start() called but task is already running.");
      return;
    }
    auto task_logic = [this](){ this->taskLoop(); };
    std::string task_name = "hand_task";
    _logger.i(TAG, "ClockHand::start() - Creating task '%s'.", task_name.c_str());
    _task = _systemOnChip.createTask(task_logic, task_name, TASK_STACK_SIZE_BYTES);
  }

  void ClockHand::resume()
  {
      _logger.i(TAG, "ClockHand::resume() - Resuming task (skipping homing).");
      _skipHomingOnNextStart = true;
      start(); 
  }

  void ClockHand::stop()
  {
      _logger.i(TAG, "ClockHand::stop() - Stopping task.");
      _shouldStop = true;
      if (_task) {
          _task.reset();
      }

      if (_stepperMotor)
      {
          _logger.i(TAG, "ClockHand::stop() - Ensuring motor is stopped and IDLE.");
          _stepperMotor->stopBlocking();
      }

      _shouldStop = false; // Reset for next start
  }

  void ClockHand::moveToValue(int value) 
  {
      if (_task) {
          _logger.w(TAG, "moveToValue called while task is running. Stop task first.");
          return;
      }
      moveToDisplayValue(value);

      // Since there is no task running to drive the motor, we must do it synchronously.
      // We use sleep(0) to yield but poll as fast as possible.
      while (_stepperMotor->isBusy())
      {
          _stepperMotor->update();
          _systemOnChip.getSleeper().sleep(0); 
      }
  }

  void ClockHand::taskLoop()
  {
    if (!_stepperMotor)
    {
      _logger.e(TAG, "ClockHand Task: No stepper motor, task cannot run.");
      return;
    }

    bool homingSuccessful = true;
    if (_skipHomingOnNextStart) {
         _logger.i(TAG, "ClockHand Task: Skipping homing sequence as requested.");
         _skipHomingOnNextStart = false; // Reset flag
         // We assume the motor is already enabled and at a known position from previous run
         // But to be safe, we re-enable it.
         _stepperMotor->enable();
    } else {
         homingSuccessful = runHomingSequence();
    }

    if (homingSuccessful)
    {
      if (setInitialPosition())
      {
        runTickingLoop();
      }
    }

    _logger.i(TAG, "ClockHand Task: Stopped.");
  }

  bool ClockHand::runHomingSequence()
  {
    auto &sleeper = _systemOnChip.getSleeper();
    _logger.i(TAG, "Homing: Starting homing sequence.");
    _logger.d(TAG, "Homing: Enabling motor.");
    _stepperMotor->enable();
    if (!_stepperMotor->home())
    {
      _logger.e(TAG, "Homing: Failed to send homing command.");
      return false;
    }

    _logger.d(TAG, "Homing: Waiting for motor to home...");
    while (!_shouldStop.load() && (_stepperMotor->needsHoming() || _stepperMotor->isBusy()))
    {
      _stepperMotor->update();
      sleeper.sleep(1);
    }

    if (_shouldStop.load())
    {
      _logger.w(TAG, "Homing: Task stopped during homing sequence.");
      return false;
    }

    if (_stepperMotor->isHomingFailed())
    {
      _logger.e(TAG, "Homing: Motor homing failed.");
      return false;
    }

    _logger.i(TAG, "Homing: Motor homing successful.");
    return true;
  }

  bool ClockHand::setInitialPosition()
  {
    auto &sleeper = _systemOnChip.getSleeper();
    _logger.i(TAG, "Initial Position: Setting to current time.");
    soc::api::ITime::TimeComponents currentTime;
    if (!_timeProvider.asTimeComponents(currentTime))
    {
      _logger.e(TAG, "Initial Position: Could not get current time components.");
      return false;
    }

    int targetDisplayValue = 0;
    switch (_type)
    {
    case HandType::HOUR:
      targetDisplayValue = currentTime.hours % 12;
      break;
    case HandType::MINUTE:
      targetDisplayValue = currentTime.minutes;
      break;
    case HandType::SECOND:
      targetDisplayValue = currentTime.seconds;
      break;
    }
    _logger.d(TAG, "Initial Position: Moving to %d.", targetDisplayValue);
    moveToDisplayValue(targetDisplayValue);
    _lastDisplayValue = targetDisplayValue;

    _logger.d(TAG, "Initial Position: Waiting for initial move to complete.");
    while (!_shouldStop.load() && _stepperMotor->isBusy())
    {
      _stepperMotor->update();
      sleeper.sleep(5);
    }

    if (_shouldStop.load())
    {
      _logger.w(TAG, "Initial Position: Task stopped during initial move.");
      return false;
    }

    _logger.i(TAG, "Initial Position: Initial position set to %d.", targetDisplayValue);
    return true;
  }

  void ClockHand::runTickingLoop()
  {
    auto &sleeper = _systemOnChip.getSleeper();
    _logger.i(TAG, "Ticking Loop: Entering normal operation loop.");
    while (!_shouldStop.load())
    {
      _stepperMotor->update();

      soc::api::ITime::TimeComponents time;
      if (_timeProvider.asTimeComponents(time))
      {
        int targetDisplayValue = 0;
        switch (_type)
        {
        case HandType::HOUR:
          targetDisplayValue = time.hours % 12;
          break;
        case HandType::MINUTE:
          targetDisplayValue = time.minutes;
          break;
        case HandType::SECOND:
          targetDisplayValue = time.seconds;
          break;
        }

        if (targetDisplayValue != _lastDisplayValue)
        {
          _logger.d(TAG, "Ticking Loop: Value changed from %d to %d.", _lastDisplayValue, targetDisplayValue);
          if (!_stepperMotor->isBusy())
          {
            int maxUnits = (_type == HandType::HOUR) ? 12 : 60;
            bool isWrapAround = (_lastDisplayValue == maxUnits - 1) && (targetDisplayValue == 0);

            // SPECIAL HANDLING FOR TRANSITION FROM 59 to 0 (or 11 to 0)
            // WHY IS THIS NECESSARY?
            // The Stepper library (AccelStepper) does not think in circles, but on an
            // infinitely long, linear axis of "steps" (like a tape measure). When the
            // hand is at second 59, its internal counter is at a high value.
            // The command to go to second 0 means for the library "go to step 0".
            // The shortest way there is backwards, which leads to unwanted movement.
            if (isWrapAround)
            {
              _logger.i(TAG, "Ticking Loop: Wrap-around detected. Performing fast clockwise jump.");
              
              // STEP 1: Force jump across the gap in clockwise direction.
              // We specify 360 degrees as the target. This is the same physical position
              // for the motor as 0 degrees, but forces it to take the full path clockwise
              // to get there from its current position (~325 degrees).
              const double JUMP_SPEED_MULTIPLIER = 2.5;
              _stepperMotor->setSpeed(_motorSpeedDegreesPerSecond * JUMP_SPEED_MULTIPLIER);
              _stepperMotor->moveToAbsolute(360.0);

              // STEP 2: Wait until the movement is completed.
              while (!_shouldStop.load() && _stepperMotor->isBusy())
              {
                _stepperMotor->update();
                sleeper.sleep(1);
              }

              // Reset speed to normal value.
              _stepperMotor->setSpeed(_motorSpeedDegreesPerSecond);

              // STEP 3: Reset internal position of the motor.
              // After the jump is completed, the motor "thinks" it is at 360 degrees.
              // We have to tell it: "This position is now the beginning (0 degrees)."
              // Without this reset, the next movement (to second 1) would be wrong.
              if (!_shouldStop.load()) {
                _logger.i(TAG, "Ticking Loop: Resetting motor position to 0 after jump.");
                _stepperMotor->setCurrentPositionDegrees(0.0);
              }
            }
            else
            {
              // Use standard logic for all "normal" movements (e.g. 25->26).
              moveToDisplayValue(targetDisplayValue);
            }
            _lastDisplayValue = targetDisplayValue;
          }
          else
          {
            _logger.d(TAG, "Ticking Loop: Motor busy, skipping move for value %d.", targetDisplayValue);
          }
        }
      } else {
        _logger.e(TAG, "Ticking Loop: Could not get time components.");
      }
      sleeper.sleep(1);
    }
    _logger.i(TAG, "Ticking Loop: Exiting normal operation loop.");
  }

  void ClockHand::moveToDisplayValue(int displayValue)
  {
    if (!_stepperMotor)
    {
        _logger.e(TAG, "moveToDisplayValue: No stepper motor available.");
        return;
    }

    int valueForCalc = displayValue;
    if (_type == HandType::HOUR && displayValue == 0)
    {
      valueForCalc = 12;
    }

    double targetAngle = (static_cast<double>(valueForCalc) * _degreesPerUnit);
    if (!_stepperMotor->moveToAbsolute(targetAngle))
    {
      _logger.e(TAG, "moveToDisplayValue: command failed for value %d (Angle: %.2f).", displayValue, targetAngle);
    } else {
      _logger.d(TAG, "moveToDisplayValue: command issued for value %d (Angle: %.2f).", displayValue, targetAngle);
    }
  }
} // namespace aviator_clock