#include "platform/motor/StepperSubsystem.h"
#include "platform/sensor/GpioDigitalInput.h"

namespace aviator::platform::motor {

static const char* TAG = "StepperSubsystem";

StepperSubsystem::StepperSubsystem(
    StepperHandActuator& hourHand,
    StepperHandActuator& minuteHand,
    StepperHandActuator& secondHand,
    logging::Logger* logger)
    : _logger(logger != nullptr ? *logger : _defaultLogger),
      _hourHandActuator(hourHand),
      _minuteHandActuator(minuteHand),
      _secondHandActuator(secondHand) {}

#if defined(AVIATOR_ACCEL_STEPPER_AVAILABLE)
StepperSubsystem::StepperSubsystem(
    const HardwareConfig& config,
    logging::Logger* logger)
    : _logger(logger != nullptr ? *logger : _defaultLogger),
      _hourController(std::make_unique<AccelStepperWrapper>(config.hourPins.stepPin, config.hourPins.directionPin)),
      _minuteController(std::make_unique<AccelStepperWrapper>(config.minutePins.stepPin, config.minutePins.directionPin)),
      _secondController(std::make_unique<AccelStepperWrapper>(config.secondPins.stepPin, config.secondPins.directionPin)),
      _hourSensor(std::make_unique<sensor::GpioDigitalInput>(config.hourPins.hallSensorPin, true)),
      _minuteSensor(std::make_unique<sensor::GpioDigitalInput>(config.minutePins.hallSensorPin, true)),
      _secondSensor(std::make_unique<sensor::GpioDigitalInput>(config.secondPins.hallSensorPin, true)),
      _hourHoming(std::make_unique<PreciseOneWayHomingStrategy>(*_hourController, *_hourSensor, config.homingConfig, &_logger)),
      _minuteHoming(std::make_unique<PreciseOneWayHomingStrategy>(*_minuteController, *_minuteSensor, config.homingConfig, &_logger)),
      _secondHoming(std::make_unique<PreciseOneWayHomingStrategy>(*_secondController, *_secondSensor, config.homingConfig, &_logger)),
      _ownedHourActuator(std::make_unique<StepperHandActuator>(*_hourController, *_hourHoming, config.actuatorConfig, &_logger)),
      _ownedMinuteActuator(std::make_unique<StepperHandActuator>(*_minuteController, *_minuteHoming, config.actuatorConfig, &_logger)),
      _ownedSecondActuator(std::make_unique<StepperHandActuator>(*_secondController, *_secondHoming, config.actuatorConfig, &_logger)),
      _hourHandActuator(*_ownedHourActuator),
      _minuteHandActuator(*_ownedMinuteActuator),
      _secondHandActuator(*_ownedSecondActuator) {

    if (config.motorEnablePin != StepperHandActuator::INVALID_PIN) {
        _hourHandActuator.enable();
        _minuteHandActuator.enable();
        _secondHandActuator.enable();
    }
}
#endif

StepperSubsystem::~StepperSubsystem() {
    stop();
}

void StepperSubsystem::start() {
    if (_isRunning.load()) {
        return;
    }
    _isRunning.store(true);
    _logger.i(TAG, "Starting motor runner task...");

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    xTaskCreatePinnedToCore(
        [](void* context) {
            auto* self = static_cast<StepperSubsystem*>(context);
            self->runMotorRunnerLoop();
            vTaskDelete(NULL);
        },
        "motor_runner_task",
        4096,
        this,
        5,
        &_taskHandle,
        1
    );
#endif
}

void StepperSubsystem::stop() {
    if (!_isRunning.load()) {
        return;
    }
    _isRunning.store(false);
    _logger.i(TAG, "Stopping motor runner task...");

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    if (_taskHandle != nullptr) {
        // Yield to allow motor runner task to terminate its own loop cleanly
        vTaskDelay(pdMS_TO_TICKS(10));
        _taskHandle = nullptr;
    }
#endif

    stopAll();
}

bool StepperSubsystem::isRunning() const {
    return _isRunning.load();
}

bool StepperSubsystem::stepIteration() {
    const bool hourActive = _hourHandActuator.update();
    const bool minuteActive = _minuteHandActuator.update();
    const bool secondActive = _secondHandActuator.update();
    return hourActive || minuteActive || secondActive;
}

void StepperSubsystem::runMotorRunnerLoop() {
    _logger.i(TAG, "Motor runner task loop entered.");
    while (_isRunning.load()) {
        const bool isAnyMotorActive = stepIteration();
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        if (isAnyMotorActive) {
            taskYIELD();
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
#endif
    }
    _logger.i(TAG, "Motor runner task loop terminated.");
}

aviator::domain::HandActuator& StepperSubsystem::getHourHand() {
    return _hourHandActuator;
}

aviator::domain::HandActuator& StepperSubsystem::getMinuteHand() {
    return _minuteHandActuator;
}

aviator::domain::HandActuator& StepperSubsystem::getSecondHand() {
    return _secondHandActuator;
}

StepperHandActuator& StepperSubsystem::getHourHandActuator() {
    return _hourHandActuator;
}

StepperHandActuator& StepperSubsystem::getMinuteHandActuator() {
    return _minuteHandActuator;
}

StepperHandActuator& StepperSubsystem::getSecondHandActuator() {
    return _secondHandActuator;
}

bool StepperSubsystem::homeAll() {
    const bool hourOk = _hourHandActuator.home();
    const bool minuteOk = _minuteHandActuator.home();
    const bool secondOk = _secondHandActuator.home();
    return hourOk && minuteOk && secondOk;
}

bool StepperSubsystem::isAllHomed() const {
    return _hourHandActuator.isHomed() &&
           _minuteHandActuator.isHomed() &&
           _secondHandActuator.isHomed();
}

bool StepperSubsystem::isBusy() const {
    return _hourHandActuator.isBusy() ||
           _minuteHandActuator.isBusy() ||
           _secondHandActuator.isBusy();
}

void StepperSubsystem::stopAll() {
    _hourHandActuator.stop();
    _minuteHandActuator.stop();
    _secondHandActuator.stop();
}

} // namespace aviator::platform::motor
