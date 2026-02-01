#include "AviatorClock.h"


// --- Include all component headers that will be instantiated ---
#include <WiFi.h>  // Needed for WiFi.localIP()
#include <soc/api/IDigitalInput.h>
#include <stepper/api/IStepperController.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/accel/AccelStepperWrapper.h>
#include <stepper/accel/AccelStepperMotor.h>
#include <stepper/homing/PreciseOneWayHomingStrategy.h>
#include <buttons/impl/LongPressButton.h>
#include "aviator-clock/ClockHand.h"
#include <wifi/api/IWifiManager.h>


static const char *TAG = "AviatorClock";

namespace aviator_clock
{
    AviatorClock::AviatorClock(soc::api::ISoc &soc, wifi::api::IWifiManager &wifiManager) : 
        _soc(soc), _wifiManager(wifiManager)
    {
        _soc.getLogger().i(TAG, "AviatorClock instance created.");
    }

    AviatorClock::~AviatorClock()
    {
        _soc.getLogger().i(TAG, "AviatorClock instance destroyed.");
    }

    void AviatorClock::setup()
    {
        _soc.getLogger().i(TAG, "Setting up AviatorClock components...");

        // Create hardware-related components using the encapsulated constants
        _hallSensor = _soc.createDigitalInput(HALL_SENSOR_PIN, true /* isActiveLow */);
        _hallSensor->begin();
        _setupButtonPin = _soc.createDigitalInput(SETUP_BUTTON_PIN, true /* isActiveLow */);
        _setupButtonPin->begin();
        _setupButton = std::make_unique<LongPressButton>(_soc, *_setupButtonPin);

        _accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
        _soc.getLogger().i(TAG, "Level 0 components created.");

        // Configuration for the new Homing Strategy
        stepper::homing::PreciseOneWayHomingStrategy::Config homingConfig{
            .homingSpeed = 200.0f,
            .homingAcceleration = 1000.0f,
            .maxHomingTravel = EFFECTIVE_STEPS_PER_REVOLUTION * 2,
            .moveDirection = 1,
            .backOffDistance = 100
        };

        // Instantiate the new PreciseOneWayHomingStrategy
        _homingStrategy = std::make_unique<stepper::homing::PreciseOneWayHomingStrategy>(
            _soc.getLogger(),
            *_accelWrapper,
            *_hallSensor,
            homingConfig);

        _soc.getLogger().i(TAG, "Level 1 components (HomingStrategy) created.");

        // The enable pin is now passed to the constructor to allow for software control.
        auto motor = std::make_unique<stepper::accel::AccelStepperMotor>(
            _soc.getLogger(),
            *_accelWrapper,
            EFFECTIVE_STEPS_PER_REVOLUTION,
            *_homingStrategy,
            ENABLE_PIN_HW);

        _soc.getLogger().i(TAG, "Level 2 components (Motor) created.");

        // Create the main application component (ClockHand)
        _secondHand = std::make_unique<aviator_clock::ClockHand>(
            aviator_clock::ClockHand::HandType::SECOND,
            _soc,
            std::move(motor),
            DIAL_TOTAL_ACTIVE_ANGLE,
            SHARP_TICK_SPEED_DPS,
            SHARP_TICK_ACCELERATION_DPS2);

        _soc.getLogger().i(TAG, "Level 3 components (ClockHand) created.");

        // Setup the ClockHand itself
        if (_secondHand)
        {
            _secondHand->setup();
        }
    }

    void AviatorClock::start()
    {
        _soc.getLogger().i(TAG, "Starting AviatorClock logic...");

        // Always start the button monitoring task first, so we can detect presses even if we wait below
        _buttonTask = _soc.createTask(
            [this]()
            { this->buttonMonitoringTask(); },
            "button_task");
        _soc.getLogger().i(TAG, "Button monitoring task started.");

        if (!_wifiManager.hasCredentials())
        {
            _soc.getLogger().w(TAG, "No WiFi credentials found.");
            _soc.getLogger().i(TAG, "Waiting for Setup Button (3s press) to configure WiFi...");
            
            // Wait until we have credentials (supplied via Setup Mode)
            while (!_wifiManager.hasCredentials()) {
                 _soc.getSleeper().sleep(100);
            }
             _soc.getLogger().i(TAG, "WiFi credentials available. Proceeding...");
        }

        // Wait a bit if we are transitioning out of setup mode
        // The Setup Task handles the connection, so we don't strictly need to call begin() again if it does.
        // But to be safe and ensure consistent state if we booted with credentials:
        
        if (!_wifiManager.isSetupModeActive()) {
            // Only force connection initiate if we are NOT in setup mode.
            // If setup mode was just active, it will handle connection on exit.
            _wifiManager.begin();
        } else {
             // If setup mode IS active (e.g. user triggered it during the wait loop above?),
             // we should wait for it to finish?
             // Actually, the loop `while(!hasCredentials)` exits when credentials appear.
             // But Setup Mode might still be sending the "Saved" response HTML.
             // We should wait until Setup Mode is officially done.
             
             _soc.getLogger().i(TAG, "Waiting for Setup Mode to complete cleanup...");
             while (_wifiManager.isSetupModeActive()) {
                 _soc.getSleeper().sleep(100);
             }
             _soc.getLogger().i(TAG, "Setup Mode completed.");
             // Now we can be sure connection logic has been triggered by the Setup Task cleanup.
        }
        
        // --- 1. Wait for WiFi Connection ---
        _soc.getLogger().i(TAG, "Waiting for WiFi connection...");
        unsigned long startWait = _soc.getTimeProvider().millis();
        // Give it up to 15 seconds to connect
        while (!_wifiManager.isConnected() && (_soc.getTimeProvider().millis() - startWait < 15000)) {
            _soc.getSleeper().sleep(100);
        }

        if (_wifiManager.isConnected()) {
             _soc.getLogger().i(TAG, "WiFi Connected! IP: %s", WiFi.localIP().toString().c_str());
             
             // --- 2. Wait for NTP Sync ---
             _soc.getLogger().i(TAG, "Waiting for NTP time sync...");
             unsigned long startNtp = _soc.getTimeProvider().millis();
             // Give it up to 20 seconds to sync time
             while (_soc.getTimeProvider().now() == 0 && (_soc.getTimeProvider().millis() - startNtp < 20000)) {
                 _soc.getSleeper().sleep(100);
             }
             
             if (_soc.getTimeProvider().now() > 0) {
                 _soc.getLogger().i(TAG, "NTP System Time is valid. Starting motors.");
             } else {
                 _soc.getLogger().w(TAG, "NTP Time sync timed out (20s). Starting motors with invalid time.");
             }

        } else {
             _soc.getLogger().w(TAG, "WiFi connection timed out (15s). Starting motors in offline mode.");
        }
        
        _startNormalOperation();
    }

    bool AviatorClock::_checkForSetupModeActivationAtBoot()
    {
        // This method is now legacy/redundant if we use the button task immediately, 
        // but can be kept if we want a specific 'boot window'. 
        // For now, I replaced the call in start() with the new logic.
        return false;
    }

    void AviatorClock::_startNormalOperation()
    {
        if (_secondHand)
        {
            _secondHand->start();
            _soc.getLogger().i(TAG, "ClockHand task started.");
        }
    }

    void AviatorClock::_enterWifiSetupMode()
    {
        if (_wifiManager.isSetupModeActive()) return;

        _soc.getLogger().i(TAG, "Entering WiFi Setup Mode...");
        _wifiManager.startSetupMode();
        
        // Setup Mode runs in its own task within WifiManager. 
        // We continue execution here.
    }


    void AviatorClock::buttonMonitoringTask()
    {
        _soc.getLogger().i(TAG, "Button monitoring task running.");
        while (true)
        {
            _setupButton->update();
            
            if (_setupButton->wasLongPressed())
            {
                // 3 seconds passed.
                _soc.getLogger().w(TAG, "Long press detected (3s).");
                
                // Check if user keeps holding for factory reset (total 10s)
                // We already waited 3s. We wait 7s more.
                // We check every 100ms if button is still active.
                
                unsigned long startCheck = _soc.getTimeProvider().millis();
                bool resetTriggered = false;
                
                while (_setupButtonPin->isActive()) {
                    unsigned long now = _soc.getTimeProvider().millis();
                    
                    // Feedback every second
                    if ((now - startCheck) % 1000 == 0) {
                         // Optional: flash LED or log
                    }

                    if (now - startCheck > 7000) {
                        resetTriggered = true;
                        break; 
                    }
                    _soc.getSleeper().sleep(50);
                }

                if (resetTriggered) {
                    _soc.getLogger().e(TAG, "FACTORY RESET TRIGGERED (10s press)!");
                    _wifiManager.resetCredentials();
                    _soc.getLogger().i(TAG, "Restarting system...");
                    _soc.getSleeper().sleep(1000);
                    ESP.restart();
                } else {
                    // Normal 3s action: Toggle Setup Mode
                    _soc.getLogger().i(TAG, "Toggling WiFi setup.");
                    if (_wifiManager.isSetupModeActive()) {
                        _wifiManager.stopSetupMode();
                    } else {
                        _enterWifiSetupMode();
                    }
                }
            }
            _soc.getSleeper().sleep(20); // Check every 20ms
        }
    }

} // namespace aviator_clock
