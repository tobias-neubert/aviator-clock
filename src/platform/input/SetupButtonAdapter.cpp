#include "platform/input/SetupButtonAdapter.h"

#include <ctime>

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <Arduino.h>
#endif

namespace aviator::platform::input {

SetupButtonAdapter::SetupButtonAdapter(
    domain::ClockInputEvents& clockInputEvents,
    const SetupButtonConfiguration& configuration,
    PinReader customPinReader,
    TimeSupplier customTimeSupplier)
    : _clockInputEvents(clockInputEvents),
      _configuration(configuration),
      _customPinReader(std::move(customPinReader)),
      _customTimeSupplier(std::move(customTimeSupplier)) {
}

SetupButtonAdapter::~SetupButtonAdapter() {
    stop();
}

void SetupButtonAdapter::initialize() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    pinMode(_configuration.pin, _configuration.activeLow ? INPUT_PULLUP : INPUT);
#endif
}

void SetupButtonAdapter::start() {
    if (_isRunning.load()) {
        return;
    }
    _isRunning.store(true);
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    xTaskCreatePinnedToCore(
        [](void* context) {
            static_cast<SetupButtonAdapter*>(context)->runPollingLoop();
        },
        "setup_btn_task",
        2048,
        this,
        1,
        &_taskHandle,
        1);
#endif
}

void SetupButtonAdapter::stop() {
    if (!_isRunning.load()) {
        return;
    }
    _isRunning.store(false);
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    if (_taskHandle != nullptr) {
        // Yield to allow polling task loop to terminate cleanly and delete itself
        vTaskDelay(pdMS_TO_TICKS(_configuration.pollingIntervalMilliseconds * 2));
        _taskHandle = nullptr;
    }
#endif
}

bool SetupButtonAdapter::isRunning() const {
    return _isRunning.load();
}

void SetupButtonAdapter::update() {
    const uint32_t currentTimestamp = _customTimeSupplier ? _customTimeSupplier() : getSystemMilliseconds();
    const bool isRawActive = _customPinReader ? _customPinReader() : readRawPin();
    update(isRawActive, currentTimestamp);
}

void SetupButtonAdapter::update(bool isRawActive, uint32_t currentTimestampMilliseconds) {
    switch (_state) {
        case ButtonState::Released: {
            if (isRawActive) {
                _debounceStartTimeMilliseconds = currentTimestampMilliseconds;
                if (_configuration.debounceDurationMilliseconds == 0) {
                    _state = ButtonState::Pressed;
                    _pressStartTimeMilliseconds = currentTimestampMilliseconds;
                    _factoryResetDispatched = false;
                } else {
                    _state = ButtonState::DebouncingPress;
                }
            }
            break;
        }

        case ButtonState::DebouncingPress: {
            if (!isRawActive) {
                // Mechanical contact chatter or transient EMI spike shorter than debounce duration
                _state = ButtonState::Released;
            } else if ((currentTimestampMilliseconds - _debounceStartTimeMilliseconds) >= _configuration.debounceDurationMilliseconds) {
                _state = ButtonState::Pressed;
                _pressStartTimeMilliseconds = _debounceStartTimeMilliseconds;
                _factoryResetDispatched = false;
            }
            break;
        }

        case ButtonState::Pressed: {
            // Dispatched immediately once 10 seconds of continuous hold is reached without requiring button release
            if (!_factoryResetDispatched && ((currentTimestampMilliseconds - _pressStartTimeMilliseconds) >= _configuration.factoryResetThresholdMilliseconds)) {
                _factoryResetDispatched = true;
                _clockInputEvents.onSetupButtonFactoryReset();
            }

            if (!isRawActive) {
                _releaseDebounceStartTimeMilliseconds = currentTimestampMilliseconds;
                if (_configuration.debounceDurationMilliseconds == 0) {
                    handleButtonRelease(currentTimestampMilliseconds);
                } else {
                    _state = ButtonState::DebouncingRelease;
                }
            }
            break;
        }

        case ButtonState::DebouncingRelease: {
            if (!_factoryResetDispatched && ((currentTimestampMilliseconds - _pressStartTimeMilliseconds) >= _configuration.factoryResetThresholdMilliseconds)) {
                _factoryResetDispatched = true;
                _clockInputEvents.onSetupButtonFactoryReset();
            }

            if (isRawActive) {
                // Contact chatter while releasing; button is still held down
                _state = ButtonState::Pressed;
            } else if ((currentTimestampMilliseconds - _releaseDebounceStartTimeMilliseconds) >= _configuration.debounceDurationMilliseconds) {
                handleButtonRelease(_releaseDebounceStartTimeMilliseconds);
            }
            break;
        }
    }
}

void SetupButtonAdapter::handleButtonRelease(uint32_t releaseTimestampMilliseconds) {
    _state = ButtonState::Released;

    // If 10-second factory reset was already dispatched, suppress subsequent release events
    if (_factoryResetDispatched) {
        _factoryResetDispatched = false;
        return;
    }

    const uint32_t heldDuration = (releaseTimestampMilliseconds >= _pressStartTimeMilliseconds)
                                      ? (releaseTimestampMilliseconds - _pressStartTimeMilliseconds)
                                      : 0;

    if (heldDuration >= _configuration.longPressThresholdMilliseconds) {
        _clockInputEvents.onSetupButtonLongPressed();
    } else {
        _clockInputEvents.onSetupButtonPressed();
    }
}

ButtonState SetupButtonAdapter::getState() const {
    return _state;
}

bool SetupButtonAdapter::isPressed() const {
    return _state == ButtonState::Pressed || _state == ButtonState::DebouncingRelease;
}

const SetupButtonConfiguration& SetupButtonAdapter::getConfiguration() const {
    return _configuration;
}

void SetupButtonAdapter::setPinReader(PinReader customPinReader) {
    _customPinReader = std::move(customPinReader);
}

void SetupButtonAdapter::setTimeSupplier(TimeSupplier customTimeSupplier) {
    _customTimeSupplier = std::move(customTimeSupplier);
}

void SetupButtonAdapter::runPollingLoop() {
    while (_isRunning.load()) {
        update();
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(_configuration.pollingIntervalMilliseconds));
#endif
    }
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    _taskHandle = nullptr;
    vTaskDelete(NULL);
#endif
}

bool SetupButtonAdapter::readRawPin() const {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    const int pinLevel = digitalRead(_configuration.pin);
    return _configuration.activeLow ? (pinLevel == LOW) : (pinLevel == HIGH);
#else
    return false;
#endif
}

uint32_t SetupButtonAdapter::getSystemMilliseconds() {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    return static_cast<uint32_t>(millis());
#else
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    return static_cast<uint32_t>((timeSpec.tv_sec * 1000ULL) + (timeSpec.tv_nsec / 1000000ULL));
#endif
}

} // namespace aviator::platform::input
