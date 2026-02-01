#include <aviator-clock/ClockHand.h>
#include <string>

static const char* TAG = "ClockHand";

namespace aviator_clock
{

  ClockHand::ClockHand(HandType type, soc::api::ISoc &soc,
                       std::unique_ptr<IStepperMotor> &&stepperMotor,
                       double totalAngleForDial,
                       double motorSpeedDps, double motorAccelerationDps2)
      : _type(type),
        _soc(soc),
        _logger(soc.getLogger()),
        _timeProvider(_soc.getTimeProvider()),
        _stepperMotor(std::move(stepperMotor)),
        _shouldStop(false),
        _totalAngleForDial(totalAngleForDial),
        _motorSpeedDps(motorSpeedDps),
        _motorAccelerationDps2(motorAccelerationDps2),
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
    _stepperMotor->setSpeed(_motorSpeedDps);
    _stepperMotor->setAcceleration(_motorAccelerationDps2);
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
    auto task_logic = [this]()
    { this->taskLoop(); };
    std::string task_name = "hand_task";
    _logger.i(TAG, "ClockHand::start() - Creating task '%s'.", task_name.c_str());
    _task = _soc.createTask(task_logic, task_name, 4096 * 2);
  }

  void ClockHand::taskLoop()
  {
    if (!_stepperMotor)
    {
      _logger.e(TAG, "ClockHand Task: No stepper motor, task cannot run.");
      return;
    }

    if (runHomingSequence())
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
    auto &sleeper = _soc.getSleeper();
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
    auto &sleeper = _soc.getSleeper();
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
    auto &sleeper = _soc.getSleeper();
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

            // SONDERBEHANDLUNG FÜR DEN ÜBERGANG VON 59 auf 0 (bzw. 11 auf 0)
            // WARUM IST DAS NÖTIG?
            // Die Stepper-Bibliothek (AccelStepper) denkt nicht in Kreisen, sondern auf einer
            // unendlich langen, linearen Achse von "Schritten" (wie ein Maßband). Wenn der
            // Zeiger bei Sekunde 59 steht, ist sein interner Zähler auf einem hohen Wert.
            // Der Befehl, zu Sekunde 0 zu fahren, bedeutet für die Bibliothek "gehe zu Schritt 0".
            // Der kürzeste Weg dahin ist rückwärts, was zur unerwünschten Bewegung führt.
            if (isWrapAround)
            {
              _logger.i(TAG, "Ticking Loop: Wrap-around detected. Performing fast clockwise jump.");
              
              // SCHRITT 1: Sprung über die Lücke im Uhrzeigersinn erzwingen.
              // Wir geben als Ziel 360 Grad an. Das ist für den Motor die gleiche physische
              // Position wie 0 Grad, zwingt ihn aber, den vollen Weg im Uhrzeigersinn zu nehmen,
              // um von seiner aktuellen Position (~325 Grad) dorthin zu gelangen.
              const double JUMP_SPEED_MULTIPLIER = 2.5;
              _stepperMotor->setSpeed(_motorSpeedDps * JUMP_SPEED_MULTIPLIER);
              _stepperMotor->moveToAbsolute(360.0);

              // SCHRITT 2: Warten, bis die Bewegung abgeschlossen ist.
              while (!_shouldStop.load() && _stepperMotor->isBusy())
              {
                _stepperMotor->update();
                sleeper.sleep(1);
              }

              // Geschwindigkeit wieder auf den Normalwert zurücksetzen.
              _stepperMotor->setSpeed(_motorSpeedDps);

              // SCHRITT 3: Interne Position des Motors zurücksetzen.
              // Nachdem der Sprung vollendet ist, "denkt" der Motor, er sei bei 360 Grad.
              // Wir müssen ihm sagen: "Diese Position ist ab sofort wieder der Anfang (0 Grad)."
              // Ohne diesen Reset wäre die nächste Bewegung (zu Sekunde 1) falsch.
              if (!_shouldStop.load()) {
                _logger.i(TAG, "Ticking Loop: Resetting motor position to 0 after jump.");
                _stepperMotor->setCurrentPositionDegrees(0.0);
              }
            }
            else
            {
              // Für alle "normalen" Bewegungen (z.B. 25->26) die Standard-Logik verwenden.
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