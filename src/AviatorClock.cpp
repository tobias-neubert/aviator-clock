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
#include <buttons/impl/SwitchGroup.h>
#include <soc/api/IRotaryEncoder.h>
#include <wifi/api/IWifiManager.h>

// --- App Modules (Complete Type Definitions needed for implementation) ---
#include "aviator-clock/ClockHand.h"
#include "aviator-clock/Time.h"
#include "aviator-clock/AlarmRepository.h"
#include "aviator-clock/TimeEditor.h"


static const char *TAG = "AviatorClock";

namespace aviator_clock
{
    AviatorClock::AviatorClock(soc::api::ISoc &soc, wifi::api::IWifiManager &wifiManager) : 
        _systemOnChip(soc), _wifiManager(wifiManager)
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
            _modeButton->update();

            // --- 1. Mode Switching Logic (Alarm Setup) ---
            if (_modeButton->wasLongPressed()) {
                if (_currentState == State::NormalRunning) {
                    _enterAlarmSetup();
                } else if (_currentState == State::AlarmSetup) {
                    _exitAlarmSetup();
                }
            }

            // --- 2. Alarm Setup Logic ---
            if (_currentState == State::AlarmSetup) {
                _updateAlarmSetup();
            }

            // --- 3. WiFi Setup Logic ---
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

    // --- Alarm Setup Implementation ---

    void AviatorClock::_enterAlarmSetup() {
        _systemOnChip.getLogger().i(TAG, "Entering Alarm Setup Mode");
        _currentState = State::AlarmSetup;
        
        // Stop normal ticking
        if (_secondHand) _secondHand->stop();
        
        // Reset encoders
        _minuteEncoder->reset();
        _hourEncoder->reset();
        
        // Read current slot
        _lastSelectedSlot = _switchGroup->read();
        _systemOnChip.getLogger().i(TAG, "Selected Slot: %d", _lastSelectedSlot);

        // Load time for slot
        Time t = _alarmRepository->getAlarm(_lastSelectedSlot);
        _timeEditor->setTime(t);
        
        // Move hands
        if (_secondHand) {
            // Move to AM/PM pos
            _secondHand->moveToValue(_timeEditor->getSecondHandTarget());
        }
    }

    void AviatorClock::_exitAlarmSetup() {
        _systemOnChip.getLogger().i(TAG, "Exiting Alarm Setup Mode");
        
        // Save Time from Editor back to Repository
        Time t = _timeEditor->getTime();
        _alarmRepository->setAlarm(_lastSelectedSlot, t);
        
        // Return to operational state
        _resumeNormalOperation();
    }

    void AviatorClock::_updateAlarmSetup() {
        // 1. Check Slot Change
        uint8_t currentSlot = _switchGroup->read();
        if (currentSlot != _lastSelectedSlot) {
             _systemOnChip.getLogger().i(TAG, "Slot changed to: %d", currentSlot);
             
            // Save previous
            Time t = _timeEditor->getTime();
            _alarmRepository->setAlarm(_lastSelectedSlot, t);
            
            // Load new
            _lastSelectedSlot = currentSlot;
            Time newTime = _alarmRepository->getAlarm(currentSlot);
            _timeEditor->setTime(newTime);
            
            // Update Hands
            if (_secondHand) _secondHand->moveToValue(_timeEditor->getSecondHandTarget());
            // TODO: Move Minute Hand (once available) to display newTime.minutes
        }
        
        // 2. Read Encoders
        _minuteEncoder->update();
        int32_t deltaMin = _minuteEncoder->getDelta();
        if (deltaMin != 0) {
            _timeEditor->adjustMinutes(deltaMin);
            _systemOnChip.getLogger().d(TAG, "Min Delta: %d, Value: %d", deltaMin, _timeEditor->getTime().minutes);
        }

        _hourEncoder->update();
        int32_t deltaHour = _hourEncoder->getDelta();
        if (deltaHour != 0) {
            _timeEditor->adjustHours(deltaHour);
            _systemOnChip.getLogger().d(TAG, "Hour Delta: %d, Value: %d", deltaHour, _timeEditor->getTime().hours);
        }

        // Update Hands if time changed
        if (deltaMin != 0 || deltaHour != 0) {
             // The second hand indicates AM/PM. Only move if AM/PM changed.
            if (_secondHand) _secondHand->moveToValue(_timeEditor->getSecondHandTarget());
            
            // TODO: Move Minute/Hour Hand to reflect changes
        }
    }

    void AviatorClock::_initializeHardwareInputs()
    {
        _systemOnChip.getLogger().d(TAG, "Initializing Hardware Inputs...");

        // 1. Digital Inputs & Buttons
        _hallSensor = _systemOnChip.createDigitalInput(PIN_HALL_EFFECT_SENSOR, true /* isActiveLow */);

        _setupButtonPin = _systemOnChip.createDigitalInput(PIN_SETUP_BUTTON, true /* isActiveLow */);
        _setupButton = std::make_unique<LongPressButton>(_systemOnChip, *_setupButtonPin);

        _modeButtonPin = _systemOnChip.createDigitalInput(PIN_MODE_BUTTON, true); 
        _modeButton = std::make_unique<LongPressButton>(_systemOnChip, *_modeButtonPin, SETUP_MODE_TOGGLE_MS);

        // 2. DIP Switches
        _switchBit0 = _systemOnChip.createDigitalInput(PIN_DIP_SWITCH_BIT_0, true);
        _switchBit1 = _systemOnChip.createDigitalInput(PIN_DIP_SWITCH_BIT_1, true);
        _switchBit2 = _systemOnChip.createDigitalInput(PIN_DIP_SWITCH_BIT_2, true);
        _switchGroup = std::make_unique<SwitchGroup>(*_switchBit0, *_switchBit1, *_switchBit2);

        // 3. Rotary Encoders
        _minuteEncoder = _systemOnChip.createRotaryEncoder(PIN_ENCODER_MINUTE_A, PIN_ENCODER_MINUTE_B, ENCODER_STEPS_PER_NOTCH);
        _hourEncoder = _systemOnChip.createRotaryEncoder(PIN_ENCODER_HOUR_A, PIN_ENCODER_HOUR_B, ENCODER_STEPS_PER_NOTCH);
    }

    void AviatorClock::_initializeLogicSubsystems()
    {
        _systemOnChip.getLogger().d(TAG, "Initializing Logic Subsystems...");

        _alarmRepository = std::make_unique<AlarmRepository>(_systemOnChip.getPersistentStorage());
        _alarmRepository->begin();
        
        _timeEditor = std::make_unique<TimeEditor>();
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
        _secondHand = std::make_unique<aviator_clock::ClockHand>(
            aviator_clock::ClockHand::HandType::SECOND,
            _systemOnChip,
            std::move(motor),
            DIAL_TOTAL_ACTIVE_ANGLE,
            SHARP_TICK_SPEED_DEGREES_PER_SECOND,
            SHARP_TICK_ACCELERATION_DEGREES_PER_SECOND_SQUARED);

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
