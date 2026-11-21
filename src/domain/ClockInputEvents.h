#pragma once

namespace aviator::domain {

/**
 * @brief Abstract inbound port for handling user interactions triggered via hardware inputs.
 */
class ClockInputEvents {
public:
    virtual ~ClockInputEvents() = default;

    /**
     * @brief Dispatched on short press of Setup Button.
     * Used primarily to dismiss an active ringing alarm.
     */
    virtual void onSetupButtonPressed() = 0;

    /**
     * @brief Dispatched when the Setup Button is held for at least 3 seconds.
     * Toggles Setup Mode (WiFi Access Point and Web Portal).
     */
    virtual void onSetupButtonLongPressed() = 0;

    /**
     * @brief Dispatched when the Setup Button is held for at least 10 seconds.
     * Erases stored WiFi/NVS credentials and restarts the clock.
     */
    virtual void onSetupButtonFactoryReset() = 0;
};

} // namespace aviator::domain
