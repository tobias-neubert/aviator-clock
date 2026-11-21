#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

#include "domain/ClockInputEvents.h"

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace aviator::platform::input {

/**
 * @brief Discrete internal states for the software debouncing and gesture detection state machine.
 */
enum class ButtonState {
    Released,
    DebouncingPress,
    Pressed,
    DebouncingRelease
};

/**
 * @brief Configuration parameters for the physical Setup Button.
 */
struct SetupButtonConfiguration {
    uint8_t pin{15};
    bool activeLow{true};
    uint32_t debounceDurationMilliseconds{50};
    uint32_t longPressThresholdMilliseconds{3000};
    uint32_t factoryResetThresholdMilliseconds{10000};
    uint32_t pollingIntervalMilliseconds{20};
};

using SetupButtonConfig = SetupButtonConfiguration;

/**
 * @brief Platform input adapter that monitors the single hardware Setup Button.
 * 
 * Implements non-blocking active-low digital input reading, contact debouncing, and timing
 * evaluation to distinguish short taps, 3-second mode toggles, and 10-second factory resets.
 * Forwards validated user events directly to the inbound domain port ClockInputEvents.
 */
class SetupButtonAdapter {
public:
    using TimeSupplier = std::function<uint32_t()>;
    using PinReader = std::function<bool()>;

    explicit SetupButtonAdapter(
        domain::ClockInputEvents& clockInputEvents,
        const SetupButtonConfiguration& configuration = SetupButtonConfiguration{},
        PinReader customPinReader = nullptr,
        TimeSupplier customTimeSupplier = nullptr);

    ~SetupButtonAdapter();

    /**
     * @brief Configures the hardware GPIO pin mode with internal pull-up/pull-down resistors.
     */
    void initialize();

    /**
     * @brief Starts the background FreeRTOS polling task if running on target hardware.
     */
    void start();

    /**
     * @brief Stops the background FreeRTOS polling task.
     */
    void stop();

    bool isRunning() const;

    /**
     * @brief Executes a single polling iteration using injected readers or platform defaults.
     */
    void update();

    /**
     * @brief Deterministic update step accepting explicit electrical level and monotonic timestamp.
     * Exported publicly for host-based GoogleTest verification.
     * @param isRawActive True if the electrical contact is currently closed/active.
     * @param currentTimestampMilliseconds Monotonic time in milliseconds.
     */
    void update(bool isRawActive, uint32_t currentTimestampMilliseconds);

    ButtonState getState() const;
    bool isPressed() const;
    const SetupButtonConfiguration& getConfiguration() const;

    void setPinReader(PinReader customPinReader);
    void setTimeSupplier(TimeSupplier customTimeSupplier);

private:
    void runPollingLoop();
    void handleButtonRelease(uint32_t releaseTimestampMilliseconds);
    bool readRawPin() const;
    static uint32_t getSystemMilliseconds();

    domain::ClockInputEvents& _clockInputEvents;
    SetupButtonConfiguration _configuration;
    PinReader _customPinReader{nullptr};
    TimeSupplier _customTimeSupplier{nullptr};

    ButtonState _state{ButtonState::Released};
    uint32_t _debounceStartTimeMilliseconds{0};
    uint32_t _pressStartTimeMilliseconds{0};
    uint32_t _releaseDebounceStartTimeMilliseconds{0};
    bool _factoryResetDispatched{false};
    std::atomic<bool> _isRunning{false};

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    TaskHandle_t _taskHandle{nullptr};
#endif
};

} // namespace aviator::platform::input
