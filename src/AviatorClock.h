#pragma once

#include <soc/api/ISoc.h>
#include <memory>
#include <vector>

// Forward declarations are used to avoid including full headers here.
// In C++, including a header copies its entire content. 
// Using forward declarations instead of includes has two main benefits:
// 1. Decoupling: Changes in the detailed implementation of a class (e.g., adding a 
//    private method to TimeEditor)
//    do not force a recompilation of this file and everything that includes it.
// 2. Build Time: Reduces the amount of code the compiler has to parse for each file.
// This is possible because we only use pointers or references to these types in the header,
// so the compiler only needs to know that the class DOES exist, not how big it is.
namespace soc
{
    namespace api
    {
        class IDigitalInput;
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

namespace aviator_clock
{
    // Forward declarations strictly solely for pointers/references.
    // This decouples the header from the implementation details of these classes.
    class ClockHand;
    class Time;
    class AlarmRepository;

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

        /**
         * @brief Construct a new Aviator Clock object.
         * @param systemOnChip A reference to the underlying System-on-a-Chip services.
         * @param wifiManager A reference to the WiFi Manager.
         */
        AviatorClock(soc::api::ISoc &systemOnChip, wifi::api::IWifiManager &wifiManager);

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
        // These constants define the physical wiring of the ESP32 GPIOs to the external components.
        // They are encapsulated here because the rest of the application should not care about pin numbers.
        // The values correspond to the PCB layout documentation.

        // Stepper Motor Driver Interface (A4988/DRV8825 compatible)
        static constexpr uint8_t PIN_STEPPER_DIRECTION = 12;  // Controls clockwise vs counter-clockwise rotation
        static constexpr uint8_t PIN_STEPPER_STEP_PULSE = 14; // A pulse here moves the motor one step
        static constexpr uint8_t PIN_MOTOR_ENABLE = 33;       // Active Low (usually) enable signal for the driver

        // Input Sensors
        static constexpr uint8_t PIN_HALL_EFFECT_SENSOR = 25; // Used for homing (zero position detection)
        
        // User Interface Inputs
        static constexpr uint8_t PIN_SETUP_BUTTON = 15;       // Button to trigger WiFi config mode and setup

        // --- Motor Configuration ---
        // why 1600?: 200 steps/rev * 8 microsteps (accel stepper default?) or similar.
        static constexpr int EFFECTIVE_STEPS_PER_REVOLUTION = 1600;
        
        // --- Homing (Calibration) Strategy Configuration ---
        // These constants define the physical behavior during the self-calibration phase at startup:
        // 1. Dynamics: Speed & Acceleration used while searching for the zero-point sensor.
        // 2. Safety Limits: Maximum distance to travel before giving up (prevents continuous 
        //    spinning if sensor fails).
        // 3. Direction: Defines if we search clockwise (1) or counter-clockwise (-1).
        // 4. Fine Tuning: 'Backoff' moves the motor slightly away from the sensor edge to 
        //ensure clean sensor release.
        static constexpr float HOMING_SPEED_STEPS_SEC = 200.0f;
        static constexpr float HOMING_ACCEL_STEPS_SEC2 = 1000.0f;
        static constexpr long HOMING_BACKOFF_STEPS = 100;
        static constexpr long MAX_HOMING_TRAVEL_REVOLUTIONS = 2;
        static constexpr int HOMING_DIRECTION = 1;

        // --- Clock Face Configuration ---
        // The total angle that the hand moves across the dial. For a typical clock, 
        // this would be 360 degrees.
        // However, if the clock face has a gap (e.g., between 11 and
        // 0/12, the total active angle might be less than 360. This affects how we 
        // calculate the angle per unit (e.g., per second or per minute).
        static constexpr double DIAL_TOTAL_ACTIVE_ANGLE = 330.0;

        // --- Settings for Smooth Movement ---
        static constexpr double SHARP_TICK_SPEED_DEGREES_PER_SECOND = 360.0;
        static constexpr double SHARP_TICK_ACCELERATION_DEGREES_PER_SECOND_SQUARED = 1000.0;

        enum class State
        {
            Initial,
            NormalRunning,
            WifiSetup
        };

        // --- Core Dependencies ---
        soc::api::ISoc &_systemOnChip;
        wifi::api::IWifiManager &_wifiManager;

        // --- Hardware and Logic Components ---
        std::unique_ptr<stepper::api::IStepperController> _stepperController;
        std::unique_ptr<soc::api::IDigitalInput> _hallSensor;
        std::unique_ptr<soc::api::IDigitalInput> _setupButtonPin;
        std::unique_ptr<IButton> _setupButton;
        std::unique_ptr<stepper::api::IHomingStrategy> _homingStrategy;
        std::unique_ptr<ClockHand> _secondHand;
        std::unique_ptr<soc::api::ITask> _inputMonitoringTask;

        // Logic
        std::unique_ptr<class AlarmRepository> _alarmRepository;

        State _currentState{State::Initial};

        /**
         * @brief The main loop for the input monitoring task (buttons).
         * Runs forever, checking for input events.
         */
        void _executeInputMonitoringLoop();

        /**
         * @brief Starts the normal clock operation by performing a homing sequence.
         */
        void _startNormalOperation(bool isResume = false);

        /**
         * @brief Resumes the normal clock operation without homing (assumes valid position).
         */
        void _resumeNormalOperation();

        /**
         * @brief Handles WiFi setup interactions (auto-reconnect on exit, setup button actions).
         * @param wasInSetupMode Reference to state tracking variable for detecting setup mode exit.
         */
        void _handleWifiSetupInteractions(bool &wasInSetupMode);

        // --- Initialization Helpers (Clean Code: Method Extraction) ---
        /**
         * @brief Initializes all Input/Output hardware (Buttons, Switches, Sensors).
         * This encapsulates the low-level pin mapping and object creation.
         */
        void _initializeHardwareInputs();

        /**
         * @brief Initializes the stepper motor driver and the associated homing strategy.
         */
        void _initializeMotorSubsystem();

        /**
         * @brief Initializes the pure application logic components (Repositories, Time Editor).
         */
        void _initializeLogicSubsystems();

        /**
         * @brief Composes the final ClockHand objects by injecting the prepared motors and logic.
         */
        void _composeClockHands();

        /**
         * @brief Enters the WiFi setup mode.
         */
        void _enterWifiSetupMode();

        /**
         * @brief Waits for NTP time synchronization.
         * @return true if time is valid, false on timeout.
         */
        bool _syncTime();

        // --- Startup Helpers ---

        /**
         * @brief Starts the task that monitors user inputs (buttons, encoders).
         */
        void _startInputMonitoringTask();

        /**
         * @brief Ensures WiFi credentials are actively present, blocking until they are.
         */
        void _ensureWifiCredentialsAvailable();

        /**
         * @brief Waits for the configuration/setup mode to complete if it is active.
         */
        void _waitForSetupModeCompletion();
    };

} // namespace aviator_clock
