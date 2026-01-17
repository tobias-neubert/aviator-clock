#include <aviator-clock/ClockHand.h>
#include <string>

namespace aviator_clock
{

  ClockHand::ClockHand(HandType type, soc::api::ISoc &soc,
                       std::unique_ptr<IStepperMotor> &&stepperMotor,
                       double totalAngleForDial, double dialStartOffsetDegrees,
                       double motorSpeedDps, double motorAccelerationDps2)
      : _type(type),
        _soc(soc),
        _logger(_soc.getLogger()),
        _timeProvider(_soc.getTimeProvider()),
        _stepperMotor(std::move(stepperMotor)),
        _shouldStop(false),
        _totalAngleForDial(totalAngleForDial),
        _dialStartOffsetDegrees(dialStartOffsetDegrees),
        _motorSpeedDps(motorSpeedDps),
        _motorAccelerationDps2(motorAccelerationDps2),
        _lastUnitProcessed(-1)
  {
    if (_stepperMotor == nullptr)
    {
      _logger.error("ERROR: Null stepper motor provided to ClockHand");
    }

    if (_totalAngleForDial <= 0.0 || _totalAngleForDial > 360.0)
    {
      _logger.warn("Warning: Invalid totalAngleForDial (%.2f). Defaulting to 360.0",
                   _totalAngleForDial);
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
    _stepperMotor->setSpeed(_motorSpeedDps);
    _stepperMotor->setAcceleration(_motorAccelerationDps2);
    _logger.info("ClockHand::setup() - Motor configured for %s hand.",
                 _type == HandType::SECOND ? "SECOND" : "OTHER");
  }

  void ClockHand::teardown()
  {
    _shouldStop = true; // Signal task to stop
    if (_task)
    {
      // The destruction of the ITask unique_ptr will join the thread.
      // This happens automatically when clockHand is destroyed.
      // Calling reset() here ensures it happens before the motor is disabled.
      _task.reset();
    }
    if (!_stepperMotor)
      return;
    if (_stepperMotor->isBusy())
    {
      _stepperMotor->stop();
    }
    _stepperMotor->disable();
  }

  void ClockHand::start()
  {
    if (_task)
    {
      _logger.warn("ClockHand::start() called but task is already running.");
      return;
    }
    auto task_logic = [this]()
    { this->taskLoop(); };
    std::string task_name = "hand_task"; // Simplified name
    _task = _soc.createTask(task_logic, task_name, 4096 * 2);
  }

  void ClockHand::taskLoop()
  {
    auto &sleeper = _soc.getSleeper();
    if (!_stepperMotor)
    {
      _logger.error("ClockHand Task: No stepper motor, task cannot run.");
      return;
    }

    // 1. Homing Phase (Blocking)
    _logger.info("ClockHand Task: Starting homing...");
    _stepperMotor->enable();
    if (!_stepperMotor->home())
    {
      _logger.error("ClockHand Task: Homing command failed to start.");
      return; // End task
    }

    while (!_shouldStop && (_stepperMotor->needsHoming() || _stepperMotor->isBusy()))
    {
      _stepperMotor->update();
      sleeper.sleep(5);
    }

    if (_shouldStop)
      return; // Exit if stopped during homing

    if (_stepperMotor->isHomingFailed())
    {
      _logger.error("ClockHand Task: Homing failed. Task stopping.");
      return;
    }
    _logger.info("ClockHand Task: Homing successful!");

    // 2. Initial Position Phase (Blocking)
    _logger.info("ClockHand Task: Moving to initial position...");
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

      while (!_shouldStop && _stepperMotor->isBusy())
      {
        _stepperMotor->update();
        sleeper.sleep(5);
      }
      if (_shouldStop)
        return; // Exit if stopped during init move
      _logger.info("ClockHand Task: Initial position set to %d.", currentUnit);
    }
    else
    {
      _logger.error("ClockHand Task: Could not get initial time.");
    }

    // 3. Normal Ticking Phase (Loop)
    _logger.info("ClockHand Task: Entering normal ticking loop.");
    while (!_shouldStop)
    {
      // Always call update for the motor
      _stepperMotor->update();

      soc::api::ITime::TimeComponents time;
      if (_timeProvider.asTimeComponents(time))
      {
        int currentUnit = 0;
        switch (_type)
        {
        case HandType::HOUR:
          currentUnit = time.hours % 12;
          break;
        case HandType::MINUTE:
          currentUnit = time.minutes;
          break;
        case HandType::SECOND:
          currentUnit = time.seconds;
          break;
        }

        if (currentUnit != _lastUnitProcessed)
        {
          if (!_stepperMotor->isBusy())
          {
            moveToUnit(currentUnit);
            _lastUnitProcessed = currentUnit;
          }
        }
      }
      // Sleep to yield CPU. A short sleep keeps motor updates frequent.
      sleeper.sleep(1);
    }
    _logger.info("ClockHand Task: Stopped.");
  }

  void ClockHand::moveToUnit(int unit)
  {
    if (!_stepperMotor)
      return;

    int unitForCalc = unit;
    if (_type == HandType::HOUR && unit == 0)
    {
      unitForCalc = 12;
    }

    double targetAngle = _dialStartOffsetDegrees + (static_cast<double>(unitForCalc) * _degreesPerUnit);

    _logger.debug("ClockHand: Moving to unit %d (Angle: %.2f)", unit, targetAngle);
    if (!_stepperMotor->moveToAbsolute(targetAngle))
    {
      _logger.error("ClockHand: moveToAbsolute command failed.");
    }
  }
} // namespace aviator_clock