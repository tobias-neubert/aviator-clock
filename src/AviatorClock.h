#pragma once

#include <soc/api/ISoc.h>
#include <memory>
#include <vector>

// Forward declarations to avoid including full headers here
namespace soc
{
    namespace api
    {
        class IDigitalInput;
        class IRotaryEncoder;
    }
}
namespace stepper
{
    namespace api
    {
        class IStepperController;
        class IHomingStrategy;
    }
}
namespace wifi
{
    namespace api
    {
        class IWifiManager;
    }
}
class IButton;
class SwitchGroup;
#include "aviator-clock/ClockHand.h"
#include "aviator-clock/Time.h"
#include "aviator-clock/AlarmRepository.h"
#include "aviator-clock/TimeEditor.h"

namespace aviator_clock
{
    /**
     * @brief The main application class for the Aviator Clock.
     *
     * This class acts as the "Composition Root". It is responsible for creating,
     * configuring, and wiring all components of the application, such as motors,
     * sensors, and clock hands. It encapsulates the main application logic.
     */
    class AviatorClock
    {
    public:
        // --- Constants ---
        static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;
        static constexpr unsigned long NTP_SYNC_TIMEOUT_MS = 20000;
        static constexpr unsigned long FACTORY_RESET_HOLD_MS = 10000;
        static constexpr unsigned long SETUP_MODE_TOGGLE_MS = 3000;
        static constexpr unsigned long FACTORY_RESET_EXTRA_WAIT_MS = FACTORY_RESET_HOLD_MS - SETUP_MODE_TOGGLE_MS; 
        static constexpr unsigned long BUTTON_POLL_INTERVAL_MS = 20;
        static constexpr unsigned long RESET_POLL_INTERVAL_MS = 50;
        static constexpr unsigned long RESET_FEEDBACK_INTERVAL_MS = 1000;
        static constexpr unsigned long RESTART_DELAY_MS = 1000;
        static constexpr uint8_t INVALID_SLOT = 255;

        /**
         * @brief Construct a new Aviator Clock object.
         * @param soc A reference to the underlying System-on-a-Chip services.
         * @param wifiManager A reference to the WiFi Manager.
         */
        AviatorClock(soc::api::ISoc &soc, wifi::api::IWifiManager &wifiManager);

        /**
         * @brief Destructor.
         */
        ~AviatorClock();

        /**
         * @brief Sets up all hardware components and sub-modules.
         * This is where all objects are instantiated and wired together.
         */
        void setup();

        /**
         * @brief Starts the main application logic.
         * This method contains the primary control flow, such as checking for
         * setup mode or starting the normal clock operation.
         */
        void start();

    private:
        // --- Pin and Hardware Configuration ---
        static constexpr uint8_t DIR_PIN_HW = 12;
        static constexpr uint8_t STEP_PIN_HW = 14;
        static constexpr uint8_t HALL_SENSOR_PIN = 25;
        static constexpr uint8_t ENABLE_PIN_HW = 33;
        static constexpr uint8_t SETUP_BUTTON_PIN = 15;
        
        static constexpr uint8_t MODE_BUTTON_PIN = 2;
        static constexpr uint8_t SWITCH_PIN_BIT0 = 26;
        static constexpr uint8_t SWITCH_PIN_BIT1 = 27;
        static constexpr uint8_t SWITCH_PIN_BIT2 = 32;
        static constexpr uint8_t ENCODER_MINUTE_PIN_A = 5;
        static constexpr uint8_t ENCODER_MINUTE_PIN_B = 18;
        static constexpr uint8_t ENCODER_HOUR_PIN_A = 36;
        static constexpr uint8_t ENCODER_HOUR_PIN_B = 39;
        
        static constexpr int ENCODER_STEPS = 4;

        // --- Motor Configuration ---
        static constexpr int EFFECTIVE_STEPS_PER_REVOLUTION = 1600;
        static constexpr float HOMING_SPEED_STEPS_SEC = 200.0f;
        static constexpr float HOMING_ACCEL_STEPS_SEC2 = 1000.0f;
        static constexpr long HOMING_BACKOFF_STEPS = 100;
        static constexpr long MAX_HOMING_TRAVEL_REVOLUTIONS = 2;
        static constexpr int HOMING_DIRECTION = 1;

        // --- Clock Face Configuration ---
        static constexpr double DIAL_TOTAL_ACTIVE_ANGLE = 330.0;

        // --- Settings for Smooth Movement ---
        static constexpr double SHARP_TICK_SPEED_DPS = 360.0;
        static constexpr double SHARP_TICK_ACCELERATION_DPS2 = 1000.0;

        enum class State
        {
            Initial,
            NormalRunning,
            AlarmSetup,
            WifiSetup
        };

        // --- Core Dependencies ---
        soc::api::ISoc &_soc;
        wifi::api::IWifiManager &_wifiManager;

        // --- Hardware and Logic Components ---
        std::unique_ptr<stepper::api::IStepperController> _accelWrapper;
        std::unique_ptr<soc::api::IDigitalInput> _hallSensor;
        std::unique_ptr<soc::api::IDigitalInput> _setupButtonPin;
        std::unique_ptr<IButton> _setupButton;
        std::unique_ptr<stepper::api::IHomingStrategy> _homingStrategy;
        std::unique_ptr<ClockHand> _secondHand;
        std::unique_ptr<soc::api::ITask> _buttonTask;

        // --- Alarm Setup Components ---
        std::unique_ptr<soc::api::IDigitalInput> _modeButtonPin;
        std::unique_ptr<IButton> _modeButton;
        std::unique_ptr<soc::api::IRotaryEncoder> _minuteEncoder;
        std::unique_ptr<soc::api::IRotaryEncoder> _hourEncoder;

        // Switch Group Inputs
        std::unique_ptr<soc::api::IDigitalInput> _switchBit0;
        std::unique_ptr<soc::api::IDigitalInput> _switchBit1;
        std::unique_ptr<soc::api::IDigitalInput> _switchBit2;
        std::unique_ptr<SwitchGroup> _switchGroup;

        // Logic
        std::unique_ptr<class AlarmRepository> _alarmRepo; // Forward decl was not used, header included
        std::unique_ptr<class TimeEditor> _timeEditor;

        State _currentState{State::Initial};
        uint8_t _lastSelectedSlot{INVALID_SLOT};

        /**
         * @brief The main loop for the button monitoring task.
         * Runs forever, checking for a long press to trigger a restart.
         */
        void buttonMonitoringTask();

        /**
         * @brief Checks for setup mode activation during a short window at boot.
         * @return true if setup mode should be activated, false otherwise.
         */
        bool _checkForSetupModeActivationAtBoot();

        /**
         * @brief Starts the normal clock operation.
         */
        void _startNormalOperation();

        // --- Alarm Setup Helpers ---
        void _enterAlarmSetup();
        void _updateAlarmSetup();
        void _exitAlarmSetup();

        /**
         * @brief Enters the WiFi setup mode.
         */
        void _enterWifiSetupMode();

        /**
         * @brief Attempts to connect to WiFi and sync time, then starts normal operation.
         */
        void _connectAndSync();
    };

} // namespace aviator_clock
