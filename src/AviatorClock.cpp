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
#include <wifi/api/IWifiManager.h>

// --- App Modules (Complete Type Definitions needed for implementation) ---
#include "aviator-clock/ClockHand.h"
#include "aviator-clock/Time.h"
#include "aviator-clock/AlarmRepository.h"


static const char *TAG = "AviatorClock";

namespace aviator_clock
{
    AviatorClock::AviatorClock(soc::api::ISoc &systemOnChip, wifi::api::IWifiManager &wifiManager) : 
        _systemOnChip(systemOnChip), _wifiManager(wifiManager)
    {
        _systemOnChip.getLogger().i(TAG, "AviatorClock instance created.");
    }

    AviatorClock::~AviatorClock()
    {
        _systemOnChip.getLogger().i(TAG, "AviatorClock instance destroyed.");
    }

    void AviatorClock::setup()
    {
        _systemOnChip.getLogger().i(TAG, "Setting up AviatorClock components...");
        
        _initializeHardwareInputs();
        _initializeLogicSubsystems();
        _initializeMotorSubsystem();
        _composeClockHands();

        _systemOnChip.getLogger().i(TAG, "All components initialized and wired.");
    }

    void AviatorClock::start()
    {
        _systemOnChip.getLogger().i(TAG, "Starting AviatorClock logic...");

        // Always start the input monitoring task first, so we can 
        // detect presses even if we wait below
        _startInputMonitoringTask();
        _ensureWifiCredentialsAvailable();
        _waitForSetupModeCompletion();
        
        if (_wifiManager.connect(WIFI_CONNECT_TIMEOUT_MS) && _syncTime())
        {
            _systemOnChip.getLogger().i(TAG, "System ready. Starting motors.");
            _startNormalOperation();
        }
        else
        {
            _systemOnChip.getLogger().e(TAG, "Initialization failed. Clock will NOT start.");
        }
    }

    bool AviatorClock::_syncTime()
    {
        _systemOnChip.getLogger().i(TAG, "Waiting for NTP time sync...");
        unsigned long startNtp = _systemOnChip.getTimeProvider().millis();
        
        while (_systemOnChip.getTimeProvider().now() == 0 && (_systemOnChip.getTimeProvider().millis() - startNtp < NTP_SYNC_TIMEOUT_MS)) {
             _systemOnChip.getSleeper().sleep(100);
        }
             
        if (_systemOnChip.getTimeProvider().now() > 0) {
             _systemOnChip.getLogger().i(TAG, "NTP System Time is valid.");
             return true;
        }
        
        _systemOnChip.getLogger().e(TAG, "NTP Time sync timed out.");
        return false;
    }

    void AviatorClock::_startNormalOperation(bool isResume)
    {
        _currentState = State::NormalRunning;
        if (!_secondHand) return;
        
        if (isResume) {
             _systemOnChip.getLogger().i(TAG, "Resuming normal operation (skipping homing).");
             _secondHand->resume();
        } else {
             _systemOnChip.getLogger().i(TAG, "Starting normal operation (with homing).");
             _secondHand->start();
        }
    }

    void AviatorClock::_resumeNormalOperation()
    {
        _startNormalOperation(true);
    }

    void AviatorClock::_enterWifiSetupMode()
    {
        if (_wifiManager.isSetupModeActive()) return;

        _systemOnChip.getLogger().i(TAG, "Entering WiFi Setup Mode...");
        _wifiManager.startSetupMode();
        
        // Setup Mode runs in its own task within WifiManager. 
        // We continue execution here.
    }


    void AviatorClock::_executeInputMonitoringLoop()
    {
        _systemOnChip.getLogger().i(TAG, "Input monitoring task running.");
        bool wasInSetupMode = _wifiManager.isSetupModeActive();

        while (true)
        {
            // --- Update Inputs ---
            _setupButton->update();

            // --- WiFi Setup Logic ---
            _handleWifiSetupInteractions(wasInSetupMode);

            _systemOnChip.getSleeper().sleep(BUTTON_POLL_INTERVAL_MS); // Check every 20ms
        }
    }

    void AviatorClock::_handleWifiSetupInteractions(bool &wasInSetupMode)
    {
        bool isInSetupMode = _wifiManager.isSetupModeActive();
        if (wasInSetupMode && !isInSetupMode)
        {
            _systemOnChip.getLogger().i(TAG, "Setup Mode ended detected. Attempting to connect...");
            if (_wifiManager.connect(WIFI_CONNECT_TIMEOUT_MS) && _syncTime())
            {
                _systemOnChip.getLogger().i(TAG, "Reconnection successful.");
            }
        }
        wasInSetupMode = isInSetupMode;

        if (_setupButton->wasLongPressed())
        {
            // 3 seconds passed.
            _systemOnChip.getLogger().w(TAG, "Long press detected (3s).");

            // Check if user keeps holding for factory reset (total 10s)
            // We already waited 3s. We wait 7s more.
            // We check every 100ms if button is still active.

            unsigned long startCheck = _systemOnChip.getTimeProvider().millis();
            bool resetTriggered = false;

            while (_setupButtonPin->isActive())
            {
                unsigned long now = _systemOnChip.getTimeProvider().millis();

                // Feedback every second
                if ((now - startCheck) % RESET_FEEDBACK_INTERVAL_MS == 0)
                {
                    // Optional: flash LED or log
                }

                if (now - startCheck > FACTORY_RESET_EXTRA_WAIT_MS)
                {
                    resetTriggered = true;
                    break;
                }
                _systemOnChip.getSleeper().sleep(RESET_POLL_INTERVAL_MS);
            }

            if (resetTriggered)
            {
                _systemOnChip.getLogger().e(TAG, "FACTORY RESET TRIGGERED (10s press)!");
                _wifiManager.resetCredentials();
                _systemOnChip.getLogger().i(TAG, "Restarting system...");
                _systemOnChip.getSleeper().sleep(RESTART_DELAY_MS);
                ESP.restart();
            }
            else
            {
                // Normal 3s action: Toggle Setup Mode
                _systemOnChip.getLogger().i(TAG, "Toggling WiFi setup.");
                if (_wifiManager.isSetupModeActive())
                {
                    _wifiManager.stopSetupMode();
                    // Connection attempt will be triggered by the polling loop above (falling edge detection)
                }
                else
                {
                    _enterWifiSetupMode();
                }
            }
        }
    }

    void AviatorClock::_initializeHardwareInputs()
    {
        _systemOnChip.getLogger().d(TAG, "Initializing Hardware Inputs...");

        // Digital Inputs & Buttons
        _hallSensor = _systemOnChip.createDigitalInput(PIN_HALL_EFFECT_SENSOR, true /* isActiveLow */);

        _setupButtonPin = _systemOnChip.createDigitalInput(PIN_SETUP_BUTTON, true /* isActiveLow */);
        _setupButton = std::make_unique<LongPressButton>(_systemOnChip, *_setupButtonPin);
    }

    void AviatorClock::_initializeLogicSubsystems()
    {
        _systemOnChip.getLogger().d(TAG, "Initializing Logic Subsystems...");

        _alarmRepository = std::make_unique<AlarmRepository>(_systemOnChip.getPersistentStorage());
        _alarmRepository->begin();
    }

    void AviatorClock::_initializeMotorSubsystem()
    {
         _systemOnChip.getLogger().d(TAG, "Initializing Stepper Motor & Homing...");

        _stepperController = std::make_unique<stepper::accel::AccelStepperWrapper>(PIN_STEPPER_STEP_PULSE, PIN_STEPPER_DIRECTION);

        // Configuration for the new Homing Strategy
        stepper::homing::PreciseOneWayHomingStrategy::Config homingConfig{
            .homingSpeed = HOMING_SPEED_STEPS_SEC,
            .homingAcceleration = HOMING_ACCEL_STEPS_SEC2,
            .maxHomingTravel = EFFECTIVE_STEPS_PER_REVOLUTION * MAX_HOMING_TRAVEL_REVOLUTIONS,
            .moveDirection = HOMING_DIRECTION,
            .backOffDistance = HOMING_BACKOFF_STEPS
        };

        // Instantiate the new PreciseOneWayHomingStrategy
        _homingStrategy = std::make_unique<stepper::homing::PreciseOneWayHomingStrategy>(
            _systemOnChip.getLogger(),
            *_stepperController,
            *_hallSensor,
            homingConfig);
    }

    void AviatorClock::_composeClockHands()
    {
         _systemOnChip.getLogger().d(TAG, "Composing Clock Hands...");

        // The enable pin is now passed to the constructor to allow for software control.
        auto motor = std::make_unique<stepper::accel::AccelStepperMotor>(
            _systemOnChip.getLogger(),
            *_stepperController,
            EFFECTIVE_STEPS_PER_REVOLUTION,
            *_homingStrategy,
            PIN_MOTOR_ENABLE);

        // Create the main application component (ClockHand)
        _secondHand = aviator_clock::ClockHand::Builder(_systemOnChip, std::move(motor))
                          .withHandType(aviator_clock::ClockHand::HandType::SECOND)
                          .withTotalAngleForDial(DIAL_TOTAL_ACTIVE_ANGLE)
                          .withMotorSpeedDegreesPerSecond(SHARP_TICK_SPEED_DEGREES_PER_SECOND)
                          .withMotorAccelerationDegreesPerSecondSquared(SHARP_TICK_ACCELERATION_DEGREES_PER_SECOND_SQUARED)
                          .build();

        // Setup the ClockHand itself (starts its task if configured)
        if (_secondHand)
        {
            _secondHand->setup();
        }
    }

    void AviatorClock::_startInputMonitoringTask()
    {
        _systemOnChip.getLogger().i(TAG, "Starting Input Monitoring Task...");

        _inputMonitoringTask = _systemOnChip.createTask(
            [this]()
            { this->_executeInputMonitoringLoop(); },
            "input_monitoring_task");
    }

    void AviatorClock::_ensureWifiCredentialsAvailable()
    {
        if (!_wifiManager.hasCredentials())
        {
            _systemOnChip.getLogger().w(TAG, "No WiFi credentials found.");
            _systemOnChip.getLogger().i(TAG, "Waiting for Setup Button (3s press) to configure WiFi...");
            
            // Wait until we have credentials (supplied via Setup Mode)
            while (!_wifiManager.hasCredentials()) {
                 _systemOnChip.getSleeper().sleep(100);
            }
             _systemOnChip.getLogger().i(TAG, "WiFi credentials available. Proceeding...");
        }
    }

    void AviatorClock::_waitForSetupModeCompletion()
    {
        if (_wifiManager.isSetupModeActive()) 
        {
             _systemOnChip.getLogger().i(TAG, "Setup Mode is active. Waiting for completion...");
             while (_wifiManager.isSetupModeActive()) {
                 _systemOnChip.getSleeper().sleep(100);
             }
             _systemOnChip.getLogger().i(TAG, "Setup Mode completed. Connecting...");
        }
    }

} // namespace aviator_clock
