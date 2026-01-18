#include <aviator-clock/ClockHand.h>
#include <string>
#include "esp_log.h"

static const char* TAG = "ClockHand";

namespace aviator_clock
{

  ClockHand::ClockHand(HandType type, soc::api::ISoc &soc,
                       std::unique_ptr<IStepperMotor> &&stepperMotor,
                       double totalAngleForDial, double dialStartOffsetDegrees,
                       double motorSpeedDps, double motorAccelerationDps2)
      : _type(type),
        _soc(soc),
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
      ESP_LOGE(TAG, "ERROR: Null stepper motor provided to ClockHand");
    }

    if (_totalAngleForDial <= 0.0 || _totalAngleForDial > 360.0)
    {
      ESP_LOGW(TAG, "Warning: Invalid totalAngleForDial (%.2f). Defaulting to 360.0",
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
    ESP_LOGI(TAG, "ClockHand::setup() - Motor configured for %s hand.",
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
    ESP_LOGI(TAG, "ClockHand::teardown() - Resources released.");
  }

  void ClockHand::start()
  {
    if (_task)
    {
      ESP_LOGW(TAG, "ClockHand::start() called but task is already running.");
      return;
    }
    auto task_logic = [this]()
    { this->taskLoop(); };
    std::string task_name = "hand_task"; // Simplified name
    ESP_LOGI(TAG, "ClockHand::start() - Creating task '%s'.", task_name.c_str());
    _task = _soc.createTask(task_logic, task_name, 4096 * 2);
  }

  void ClockHand::taskLoop()
  {
    if (!_stepperMotor)
    {
      ESP_LOGE(TAG, "ClockHand Task: No stepper motor, task cannot run.");
      return;
    }

    if (runHomingSequence())
    {
      if (setInitialPosition())
      {
        runTickingLoop();
      }
    }

    ESP_LOGI(TAG, "ClockHand Task: Stopped.");
  }

  bool ClockHand::runHomingSequence()
  {
    auto &sleeper = _soc.getSleeper();
    ESP_LOGI(TAG, "Homing: Starting homing sequence.");
    ESP_LOGD(TAG, "Homing: Enabling motor.");
    _stepperMotor->enable();
    if (!_stepperMotor->home())
    {
      ESP_LOGE(TAG, "Homing: Failed to send homing command.");
      return false; // Homing couldn't even start
    }

    // Wait for homing to complete
    ESP_LOGD(TAG, "Homing: Waiting for motor to home...");
    while (!_shouldStop.load() && (_stepperMotor->needsHoming() || _stepperMotor->isBusy()))
    {
      ESP_LOGV(TAG, "Homing: In progress (needsHoming=%d, isBusy=%d, shouldStop=%d).", _stepperMotor->needsHoming(), _stepperMotor->isBusy(), _shouldStop.load());
      _stepperMotor->update();
      sleeper.sleep(5);
    }

    if (_shouldStop.load())
    {
      ESP_LOGW(TAG, "Homing: Task stopped during homing sequence.");
      return false; // Task was stopped during homing
    }

    if (_stepperMotor->isHomingFailed())
    {
      ESP_LOGE(TAG, "Homing: Motor homing failed.");
      return false;
    }

    ESP_LOGI(TAG, "Homing: Motor homing successful.");
    return true;
  }

  bool ClockHand::setInitialPosition()
  {
    auto &sleeper = _soc.getSleeper();
    ESP_LOGI(TAG, "Initial Position: Setting to current time.");
    soc::api::ITime::TimeComponents currentTime;
    if (!_timeProvider.asTimeComponents(currentTime))
    {
      ESP_LOGE(TAG, "Initial Position: Could not get current time components.");
      return false; // Cannot set initial position without time
    }

    int currentUnit = 0;
    switch (_type)
    {
    case HandType::HOUR:
      currentUnit = currentTime.hours % 12;
      ESP_LOGD(TAG, "Initial Position: Current hour unit is %d.", currentUnit);
      break;
    case HandType::MINUTE:
      currentUnit = currentTime.minutes;
      ESP_LOGD(TAG, "Initial Position: Current minute unit is %d.", currentUnit);
      break;
    case HandType::SECOND:
      currentUnit = currentTime.seconds;
      ESP_LOGD(TAG, "Initial Position: Current second unit is %d.", currentUnit);
      break;
    }
    ESP_LOGD(TAG, "Initial Position: Calling moveToUnit for unit %d.", currentUnit);
    moveToUnit(currentUnit);
    _lastUnitProcessed = currentUnit;

    // Wait for the move to complete
    ESP_LOGD(TAG, "Initial Position: Waiting for initial move to complete.");
    while (!_shouldStop.load() && _stepperMotor->isBusy())
    {
      ESP_LOGV(TAG, "Initial Position: Waiting for move (isBusy=%d, shouldStop=%d).", _stepperMotor->isBusy(), _shouldStop.load());
      _stepperMotor->update();
      sleeper.sleep(5);
    }

    if (_shouldStop.load())
    {
      ESP_LOGW(TAG, "Initial Position: Task stopped during initial move.");
      return false; // Task was stopped during the initial move
    }

    ESP_LOGI(TAG, "Initial Position: Initial position set to %d.", currentUnit);
    return true;
  }

  void ClockHand::runTickingLoop()
  {
    auto &sleeper = _soc.getSleeper();
    ESP_LOGI(TAG, "Ticking Loop: Entering normal operation loop.");
    while (!_shouldStop.load())
    {
      ESP_LOGV(TAG, "Ticking Loop: Iteration (shouldStop=%d).", _shouldStop.load());
      // Always call update for the motor
      ESP_LOGV(TAG, "Ticking Loop: Motor update called.");
      _stepperMotor->update();

      soc::api::ITime::TimeComponents time;
      if (_timeProvider.asTimeComponents(time))
      {
        ESP_LOGV(TAG, "Ticking Loop: Time components retrieved (H:%d, M:%d, S:%d).", time.hours, time.minutes, time.seconds);
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
          ESP_LOGD(TAG, "Ticking Loop: Unit changed from %d to %d.", _lastUnitProcessed, currentUnit);
          if (!_stepperMotor->isBusy())
          {
            ESP_LOGD(TAG, "Ticking Loop: Motor not busy, moving to new unit %d.", currentUnit);
            moveToUnit(currentUnit);
            _lastUnitProcessed = currentUnit;
          } else {
            ESP_LOGD(TAG, "Ticking Loop: Motor busy, skipping move for unit %d.", currentUnit);
          }
        }
      } else {
        ESP_LOGE(TAG, "Ticking Loop: Could not get time components.");
      }
      // Sleep to yield CPU. A short sleep keeps motor updates frequent.
      sleeper.sleep(1);
    }
    ESP_LOGI(TAG, "Ticking Loop: Exiting normal operation loop.");
  }

  void ClockHand::moveToUnit(int unit)
  {
    if (!_stepperMotor)
    {
        ESP_LOGE(TAG, "MoveToUnit: No stepper motor available.");
        return;
    }

    int unitForCalc = unit;
    if (_type == HandType::HOUR && unit == 0)
    {
      unitForCalc = 12;
    }

    double targetAngle = _dialStartOffsetDegrees + (static_cast<double>(unitForCalc) * _degreesPerUnit);

    ESP_LOGD(TAG, "MoveToUnit: Moving to unit %d (Angle: %.2f)", unit, targetAngle);
    if (!_stepperMotor->moveToAbsolute(targetAngle))
    {
      ESP_LOGE(TAG, "MoveToUnit: moveToAbsolute command failed for unit %d (Angle: %.2f).", unit, targetAngle);
    } else {
      ESP_LOGI(TAG, "MoveToUnit: Move command issued for unit %d (Angle: %.2f).", unit, targetAngle);
    }
  }
} // namespace aviator_clock