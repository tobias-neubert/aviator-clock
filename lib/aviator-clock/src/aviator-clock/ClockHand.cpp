#include <aviator-clock/ClockHand.h>
#include <string>
#include "esp_log.h"

static const char* TAG = "ClockHand";

namespace aviator_clock
{

  ClockHand::ClockHand(HandType type, soc::api::ISoc &soc,
                       std::unique_ptr<IStepperMotor> &&stepperMotor,
                       double totalAngleForDial,
                       double motorSpeedDps, double motorAccelerationDps2)
      : _type(type),
        _soc(soc),
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
    ESP_LOGI(TAG, "ClockHand::start() - Starting task for %s hand.",
                 _type == HandType::SECOND ? "SECOND" : "OTHER");
    if (_task)
    {
      ESP_LOGW(TAG, "ClockHand::start() called but task is already running.");
      return;
    }
    auto task_logic = [this]()
    { this->taskLoop(); };
    std::string task_name = "hand_task";
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
      return false;
    }

    ESP_LOGD(TAG, "Homing: Waiting for motor to home...");
    while (!_shouldStop.load() && (_stepperMotor->needsHoming() || _stepperMotor->isBusy()))
    {
      _stepperMotor->update();
      sleeper.sleep(1);
    }

    if (_shouldStop.load())
    {
      ESP_LOGW(TAG, "Homing: Task stopped during homing sequence.");
      return false;
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
    ESP_LOGD(TAG, "Initial Position: Moving to %d.", targetDisplayValue);
    moveToDisplayValue(targetDisplayValue);
    _lastDisplayValue = targetDisplayValue;

    ESP_LOGD(TAG, "Initial Position: Waiting for initial move to complete.");
    while (!_shouldStop.load() && _stepperMotor->isBusy())
    {
      _stepperMotor->update();
      sleeper.sleep(5);
    }

    if (_shouldStop.load())
    {
      ESP_LOGW(TAG, "Initial Position: Task stopped during initial move.");
      return false;
    }

    ESP_LOGI(TAG, "Initial Position: Initial position set to %d.", targetDisplayValue);
    return true;
  }

  void ClockHand::runTickingLoop()
  {
    auto &sleeper = _soc.getSleeper();
    ESP_LOGI(TAG, "Ticking Loop: Entering normal operation loop.");
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
          ESP_LOGD(TAG, "Ticking Loop: Value changed from %d to %d.", _lastDisplayValue, targetDisplayValue);
          if (!_stepperMotor->isBusy())
          {
            moveToDisplayValue(targetDisplayValue);
            _lastDisplayValue = targetDisplayValue;
          } else {
            ESP_LOGD(TAG, "Ticking Loop: Motor busy, skipping move for value %d.", targetDisplayValue);
          }
        }
      } else {
        ESP_LOGE(TAG, "Ticking Loop: Could not get time components.");
      }
      sleeper.sleep(1);
    }
    ESP_LOGI(TAG, "Ticking Loop: Exiting normal operation loop.");
  }

  void ClockHand::moveToDisplayValue(int value)
  {
    if (!_stepperMotor)
    {
        ESP_LOGE(TAG, "moveToDisplayValue: No stepper motor available.");
        return;
    }

    int valueForCalc = value;
    if (_type == HandType::HOUR && value == 0)
    {
      valueForCalc = 12;
    }

    double targetAngle = (static_cast<double>(valueForCalc) * _degreesPerUnit);
    if (!_stepperMotor->moveToAbsolute(targetAngle))
    {
      ESP_LOGE(TAG, "moveToDisplayValue: command failed for value %d (Angle: %.2f).", value, targetAngle);
    } else {
      ESP_LOGI(TAG, "moveToDisplayValue: command issued for value %d (Angle: %.2f).", value, targetAngle);
    }
  }
} // namespace aviator_clock