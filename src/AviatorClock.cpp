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

        // --- Alarm Setup Hardware ---
        // Mode Button (Pin 13)
        _modeButtonPin = _soc.createDigitalInput(MODE_BUTTON_PIN, true); 
        _modeButtonPin->begin();
        _modeButton = std::make_unique<LongPressButton>(_soc, *_modeButtonPin, SETUP_MODE_TOGGLE_MS); // 3s long press

        // Switches (26, 27, 32)
        _switchBit0 = _soc.createDigitalInput(SWITCH_PIN_BIT0, true); _switchBit0->begin();
        _switchBit1 = _soc.createDigitalInput(SWITCH_PIN_BIT1, true); _switchBit1->begin();
        _switchBit2 = _soc.createDigitalInput(SWITCH_PIN_BIT2, true); _switchBit2->begin();
        _switchGroup = std::make_unique<SwitchGroup>(*_switchBit0, *_switchBit1, *_switchBit2);

        // Rotary Encoders
        _minuteEncoder = _soc.createRotaryEncoder(ENCODER_MINUTE_PIN_A, ENCODER_MINUTE_PIN_B, ENCODER_STEPS);
        _hourEncoder = _soc.createRotaryEncoder(ENCODER_HOUR_PIN_A, ENCODER_HOUR_PIN_B, ENCODER_STEPS);

        // --- Logic Components ---
        _alarmRepo = std::make_unique<AlarmRepository>(_soc.getPersistentStorage());
        _alarmRepo->begin();
        
        _timeEditor = std::make_unique<TimeEditor>();

        _accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
        _soc.getLogger().i(TAG, "Level 0 components created.");

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
             // Just booted normally with credentials
             _connectAndSync();
        } else {
             _soc.getLogger().i(TAG, "Setup Mode is active. Waiting for completion...");
             while (_wifiManager.isSetupModeActive()) {
                 _soc.getSleeper().sleep(100);
             }
             _soc.getLogger().i(TAG, "Setup Mode completed. Connecting...");
             _connectAndSync();
        }
    }
    
    void AviatorClock::_connectAndSync() {
        // --- 1. Wait for WiFi Connection ---
        if (!_wifiManager.isConnected()) {
             _wifiManager.begin();
        }

        _soc.getLogger().i(TAG, "Waiting for WiFi connection...");
        unsigned long startWait = _soc.getTimeProvider().millis();
        // Give it up to 15 seconds to connect
        while (!_wifiManager.isConnected() && (_soc.getTimeProvider().millis() - startWait < WIFI_CONNECT_TIMEOUT_MS)) {
            _soc.getSleeper().sleep(100);
        }

        if (_wifiManager.isConnected()) {
             _soc.getLogger().i(TAG, "WiFi Connected! IP: %s", WiFi.localIP().toString().c_str());
             
             // --- 2. Wait for NTP Sync ---
             _soc.getLogger().i(TAG, "Waiting for NTP time sync...");
             unsigned long startNtp = _soc.getTimeProvider().millis();
             // Give it up to 20 seconds to sync time
             while (_soc.getTimeProvider().now() == 0 && (_soc.getTimeProvider().millis() - startNtp < NTP_SYNC_TIMEOUT_MS)) {
                 _soc.getSleeper().sleep(100);
             }
             
             if (_soc.getTimeProvider().now() > 0) {
                 _soc.getLogger().i(TAG, "NTP System Time is valid. Starting motors.");
                 _startNormalOperation();
             } else {
                 _soc.getLogger().e(TAG, "NTP Time sync timed out (20s). Clock will NOT start.");
             }

        } else {
             _soc.getLogger().e(TAG, "WiFi connection timed out (15s). Clock will NOT start.");
        }
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
        _currentState = State::NormalRunning;
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

            // --- 3. WiFi Setup Logic (Legacy / Setup Button) ---
            bool isInSetupMode = _wifiManager.isSetupModeActive();
            if (wasInSetupMode && !isInSetupMode) {
                 _soc.getLogger().i(TAG, "Setup Mode ended detected. Attempting to connect...");
                 _connectAndSync();
            }
            wasInSetupMode = isInSetupMode;

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
                    if ((now - startCheck) % RESET_FEEDBACK_INTERVAL_MS == 0) {
                         // Optional: flash LED or log
                    }

                    if (now - startCheck > FACTORY_RESET_EXTRA_WAIT_MS) {
                        resetTriggered = true;
                        break; 
                    }
                    _soc.getSleeper().sleep(RESET_POLL_INTERVAL_MS);
                }

                if (resetTriggered) {
                    _soc.getLogger().e(TAG, "FACTORY RESET TRIGGERED (10s press)!");
                    _wifiManager.resetCredentials();
                    _soc.getLogger().i(TAG, "Restarting system...");
                    _soc.getSleeper().sleep(RESTART_DELAY_MS);
                    ESP.restart();
                } else {
                    // Normal 3s action: Toggle Setup Mode
                    _soc.getLogger().i(TAG, "Toggling WiFi setup.");
                    if (_wifiManager.isSetupModeActive()) {
                        _wifiManager.stopSetupMode();
                        // Connection attempt will be triggered by the polling loop above (falling edge detection)
                    } else {
                        _enterWifiSetupMode();
                    }
                }
            }
            _soc.getSleeper().sleep(BUTTON_POLL_INTERVAL_MS); // Check every 20ms
        }
    }

    // --- Alarm Setup Implementation ---

    void AviatorClock::_enterAlarmSetup() {
        _soc.getLogger().i(TAG, "Entering Alarm Setup Mode");
        _currentState = State::AlarmSetup;
        
        // Stop normal ticking
        if (_secondHand) _secondHand->stop();
        
        // Reset encoders
        _minuteEncoder->reset();
        _hourEncoder->reset();
        
        // Read current slot
        _lastSelectedSlot = _switchGroup->read();
        _soc.getLogger().i(TAG, "Selected Slot: %d", _lastSelectedSlot);

        // Load time for slot
        Time t = _alarmRepo->getAlarm(_lastSelectedSlot);
        _timeEditor->setTime(t);
        
        // Move hands
        if (_secondHand) {
            // Move to AM/PM pos
            _secondHand->moveToValue(_timeEditor->getSecondHandTarget());
        }
    }

    void AviatorClock::_exitAlarmSetup() {
        _soc.getLogger().i(TAG, "Exiting Alarm Setup Mode");
        _currentState = State::NormalRunning;
        
        // Save current slot
        Time t = _timeEditor->getTime();
        _alarmRepo->setAlarm(_lastSelectedSlot, t);
        
        // Resume normal operation
        // This restarts the task, but we use resume() to skip homing
        if (_secondHand) {
            _secondHand->resume();
            _soc.getLogger().i(TAG, "ClockHand task resumed.");
        }
    }

    void AviatorClock::_updateAlarmSetup() {
        // 1. Check Slot Change
        uint8_t currentSlot = _switchGroup->read();
        if (currentSlot != _lastSelectedSlot) {
             _soc.getLogger().i(TAG, "Slot changed to: %d", currentSlot);
             
            // Save previous
            Time t = _timeEditor->getTime();
            _alarmRepo->setAlarm(_lastSelectedSlot, t);
            
            // Load new
            _lastSelectedSlot = currentSlot;
            Time newTime = _alarmRepo->getAlarm(currentSlot);
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
            _soc.getLogger().d(TAG, "Min Delta: %d, Value: %d", deltaMin, _timeEditor->getTime().minutes);
        }

        _hourEncoder->update();
        int32_t deltaHour = _hourEncoder->getDelta();
        if (deltaHour != 0) {
            _timeEditor->adjustHours(deltaHour);
            _soc.getLogger().d(TAG, "Hour Delta: %d, Value: %d", deltaHour, _timeEditor->getTime().hours);
        }

        // Update Hands if time changed
        if (deltaMin != 0 || deltaHour != 0) {
             // The second hand indicates AM/PM. Only move if AM/PM changed.
            if (_secondHand) _secondHand->moveToValue(_timeEditor->getSecondHandTarget());
            
            // TODO: Move Minute/Hour Hand to reflect changes
        }
    }

} // namespace aviator_clock
