#pragma once

namespace aviator::domain {

class SystemGateway;

/**
 * @brief High-level operating modes for the Aviator Clock.
 */
enum class OperatingMode {
    Initial,
    NormalRunning,
    SetupMode
};

/**
 * @brief Interface for observing operating mode transitions without dynamic allocation.
 */
class OperatingModeObserver {
public:
    virtual ~OperatingModeObserver() = default;

    virtual void onOperatingModeChanged(OperatingMode previousMode, OperatingMode newMode) = 0;
};

/**
 * @brief Pure state machine managing operating modes (Initial, NormalRunning, SetupMode).
 */
class OperatingStateMachine {
public:
    explicit OperatingStateMachine(OperatingMode initialMode = OperatingMode::Initial);

    OperatingMode getCurrentMode() const;
    bool isInitial() const;
    bool isNormalRunning() const;
    bool isSetupMode() const;

    void setObserver(OperatingModeObserver* observer);
    void setSystemGateway(SystemGateway* systemGateway);

    void onStartupCompleted();
    void onSetupButtonLongPressed();
    void onSetupButtonFactoryReset();
    void enterSetupMode();
    void exitSetupMode();

private:
    void transitionTo(OperatingMode targetMode);

    OperatingMode _currentMode;
    OperatingModeObserver* _observer{nullptr};
    SystemGateway* _systemGateway{nullptr};
};

} // namespace aviator::domain
