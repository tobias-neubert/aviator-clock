#include "app/CompositionRoot.h"

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#endif

namespace aviator::app {

namespace {
constexpr unsigned long WIFI_CONNECTION_TIMEOUT_MS = 15000;
constexpr unsigned long CLOCK_LOOP_INTERVAL_MS = 100;
} // namespace

CompositionRoot::CompositionRoot()
    : _pinConfiguration(),
      _systemGateway(),
      _alarmStorage(),
      _timeSource(),
      _audioPlayer(platform::audio::Jq6500Configuration{
          _pinConfiguration.audio.receivePin,
          _pinConfiguration.audio.transmitPin
      }),
#if defined(AVIATOR_ACCEL_STEPPER_AVAILABLE)
      _stepperSubsystem(platform::motor::StepperSubsystemHardwareConfig{
          _pinConfiguration.hourHand,
          _pinConfiguration.minuteHand,
          _pinConfiguration.secondHand,
          _pinConfiguration.motorEnablePin
      }),
#else
      _stepperSubsystem(),
#endif
      _setupButtonAdapter(*this, platform::input::SetupButtonConfiguration{_pinConfiguration.setupButtonPin}),
      _stateMachine(domain::OperatingMode::Initial),
      _alarmCoordinator(_audioPlayer, _alarmStorage),
      _clockEngine(
          _stepperSubsystem.getHourHand(),
          _stepperSubsystem.getMinuteHand(),
          _stepperSubsystem.getSecondHand()
      ),
      _clockConfigurationService(
          _alarmCoordinator,
          _audioPlayer,
          _stateMachine,
          []() -> bool {
#if defined(ARDUINO) || defined(ESP32)
              return WiFi.status() == WL_CONNECTED;
#else
              return false;
#endif
          },
          [this]() -> bool {
              return _timeSource.isSynchronized();
          }
      ),
      _webPortalServer(_clockConfigurationService, _systemGateway) {
}

CompositionRoot::~CompositionRoot() {
    stop();
}

void CompositionRoot::setup() {
    _stateMachine.setSystemGateway(&_systemGateway);
    _stateMachine.setObserver(this);

    _alarmCoordinator.initialize();
    _audioPlayer.initialize();
    _setupButtonAdapter.initialize();
}

void CompositionRoot::start() {
    if (_isRunning.load()) {
        return;
    }
    _isRunning.store(true);

    // 1. Start input monitoring and motor runner background tasks
    _setupButtonAdapter.start();
    _stepperSubsystem.start();

    // 2. Perform zero-calibration homing across all three dial hands
    _clockEngine.homeAll();

    // 3. Connect to network if credentials are saved
    bool wifiConnected = false;
#if defined(ARDUINO) || defined(ESP32)
    Preferences preferences;
    preferences.begin("wifi-config", true);
    const String ssid = preferences.getString("ssid", "");
    const String password = preferences.getString("pass", "");
    preferences.end();

    if (ssid.length() > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        const unsigned long startMillis = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startMillis < WIFI_CONNECTION_TIMEOUT_MS)) {
            delay(100);
        }
        wifiConnected = (WiFi.status() == WL_CONNECTED);
    }
#endif

    if (wifiConnected) {
        _timeSource.initialize();
        _stateMachine.onStartupCompleted();
    } else {
        _stateMachine.enterSetupMode();
    }

    // 4. Start clock background evaluation task
#if defined(ARDUINO) || defined(ESP_PLATFORM)
    xTaskCreatePinnedToCore(
        [](void* context) {
            auto* self = static_cast<CompositionRoot*>(context);
            self->runClockLoop();
            vTaskDelete(NULL);
        },
        "clock_engine_task",
        4096,
        this,
        2,
        &_clockTaskHandle,
        1
    );
#endif
}

void CompositionRoot::stop() {
    if (!_isRunning.load()) {
        return;
    }
    _isRunning.store(false);

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    if (_clockTaskHandle != nullptr) {
        vTaskDelete(_clockTaskHandle);
        _clockTaskHandle = nullptr;
    }
#endif

    _webPortalServer.stop();
    _setupButtonAdapter.stop();
    _stepperSubsystem.stop();
}

bool CompositionRoot::isRunning() const {
    return _isRunning.load();
}

void CompositionRoot::runClockLoop() {
    while (_isRunning.load()) {
        if (_stateMachine.isNormalRunning()) {
            domain::TimeComponents components{};
            if (_timeSource.asTimeComponents(components)) {
                const domain::Time currentTime{components.hours, components.minutes, components.seconds};
                _clockEngine.updateTime(currentTime);
                _alarmCoordinator.update(currentTime);
            }
        }

#if defined(ARDUINO) || defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(CLOCK_LOOP_INTERVAL_MS));
#endif
    }
}

void CompositionRoot::onSetupButtonPressed() {
    if (_alarmCoordinator.isRinging()) {
        _alarmCoordinator.dismissAlarm();
    }
}

void CompositionRoot::onSetupButtonLongPressed() {
    _stateMachine.onSetupButtonLongPressed();
}

void CompositionRoot::onSetupButtonFactoryReset() {
    _stateMachine.onSetupButtonFactoryReset();
}

void CompositionRoot::onOperatingModeChanged(domain::OperatingMode previousMode, domain::OperatingMode newMode) {
    if (newMode == domain::OperatingMode::SetupMode) {
        _webPortalServer.start();
    } else if (newMode == domain::OperatingMode::NormalRunning) {
        _webPortalServer.stop();
    }
}

domain::OperatingStateMachine& CompositionRoot::getStateMachine() {
    return _stateMachine;
}

domain::AlarmCoordinator& CompositionRoot::getAlarmCoordinator() {
    return _alarmCoordinator;
}

domain::ClockEngine& CompositionRoot::getClockEngine() {
    return _clockEngine;
}

domain::ClockConfigurationService& CompositionRoot::getClockConfigurationService() {
    return _clockConfigurationService;
}

platform::motor::StepperSubsystem& CompositionRoot::getStepperSubsystem() {
    return _stepperSubsystem;
}

platform::web::WebPortalServer& CompositionRoot::getWebPortalServer() {
    return _webPortalServer;
}

} // namespace aviator::app
