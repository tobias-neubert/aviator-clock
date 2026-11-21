#pragma once

#include <atomic>
#include <memory>

// Domain Core & Ports
#include "domain/AlarmCoordinator.h"
#include "domain/ClockConfigurationService.h"
#include "domain/ClockEngine.h"
#include "domain/ClockInputEvents.h"
#include "domain/OperatingStateMachine.h"

// Platform Subsystems & Adapters
#include "platform/audio/Jq6500AudioPlayer.h"
#include "platform/board_support_package/PinConfig.h"
#include "platform/input/SetupButtonAdapter.h"
#include "platform/motor/StepperSubsystem.h"
#include "platform/storage/NvsAlarmStorage.h"
#include "platform/system/EspSystemGateway.h"
#include "platform/time/NtpTimeSource.h"
#include "platform/web/WebPortalServer.h"

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace aviator::app {

/**
 * @brief Application Composition Root and dependency injection wiring coordinator.
 * 
 * Assembles all domain core services, state machines, and hardware platform adapters.
 * Manages system initialization, FreeRTOS background task lifecycles, and event dispatching.
 */
class CompositionRoot : public domain::ClockInputEvents,
                        public domain::OperatingModeObserver {
public:
    CompositionRoot();
    ~CompositionRoot() override;

    /**
     * @brief Performs one-time hardware initialization and dependency injection assembly.
     */
    void setup();

    /**
     * @brief Starts background execution tasks, connects to Wi-Fi/NTP, and enters operational state.
     */
    void start();

    /**
     * @brief Halts background tasks and stops platform subsystems.
     */
    void stop();

    bool isRunning() const;

    // --- domain::ClockInputEvents Implementation ---
    void onSetupButtonPressed() override;
    void onSetupButtonLongPressed() override;
    void onSetupButtonFactoryReset() override;

    // --- domain::OperatingModeObserver Implementation ---
    void onOperatingModeChanged(domain::OperatingMode previousMode, domain::OperatingMode newMode) override;

    // Subsystem accessors for inspection and testing
    domain::OperatingStateMachine& getStateMachine();
    domain::AlarmCoordinator& getAlarmCoordinator();
    domain::ClockEngine& getClockEngine();
    domain::ClockConfigurationService& getClockConfigurationService();
    platform::motor::StepperSubsystem& getStepperSubsystem();
    platform::web::WebPortalServer& getWebPortalServer();

private:
    void runClockLoop();

    // Hardware pin and configuration mapping
    platform::board_support_package::HardwarePinConfig _pinConfiguration;

    // Outbound Platform Adapters
    platform::system::EspSystemGateway _systemGateway;
    platform::storage::NvsAlarmStorage _alarmStorage;
    platform::time::NtpTimeSource _timeSource;
    platform::audio::Jq6500AudioPlayer _audioPlayer;
    platform::motor::StepperSubsystem _stepperSubsystem;

    // Inbound Driving Adapters
    platform::input::SetupButtonAdapter _setupButtonAdapter;

    // Domain Core Logic
    domain::OperatingStateMachine _stateMachine;
    domain::AlarmCoordinator _alarmCoordinator;
    domain::ClockEngine _clockEngine;
    domain::ClockConfigurationService _clockConfigurationService;

    // Platform Web Server Adapter
    platform::web::WebPortalServer _webPortalServer;

    std::atomic<bool> _isRunning{false};
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    TaskHandle_t _clockTaskHandle{nullptr};
#endif
};

} // namespace aviator::app
