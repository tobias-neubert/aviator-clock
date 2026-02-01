#include "AviatorClock.h"


// --- Include all component headers that will be instantiated ---
#include <soc/api/IDigitalInput.h>
#include <stepper/api/IStepperController.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/accel/AccelStepperWrapper.h>
#include <stepper/accel/AccelStepperMotor.h>
#include <stepper/homing/PreciseOneWayHomingStrategy.h>
#include <buttons/impl/LongPressButton.h>
#include "aviator-clock/ClockHand.h"


static const char *TAG = "AviatorClock";

namespace aviator_clock
{
    AviatorClock::AviatorClock(soc::api::ISoc &soc) : 
        _soc(soc)
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

        if (_checkForSetupModeActivationAtBoot())
        {
            _enterWifiSetupMode();
        }
        else
        {
            _startNormalOperation();
        }
    }

    bool AviatorClock::_checkForSetupModeActivationAtBoot()
    {
        _soc.getLogger().i(TAG, "Checking for setup mode activation...");
        _soc.getLogger().i(TAG, "Hold button on GPIO%d for 3s to enter setup.", SETUP_BUTTON_PIN);

        const unsigned long checkDuration = 4000; // ms
        unsigned long checkStartTime = _soc.getTimeProvider().millis();

        while (_soc.getTimeProvider().millis() - checkStartTime < checkDuration)
        {
            _setupButton->update();
            if (_setupButton->wasLongPressed())
            {
                _soc.getLogger().w(TAG, "Long press detected! Entering setup mode.");
                return true;
            }
            if (_setupButton->wasShortPressed())
            {
                _soc.getLogger().i(TAG, "Short press detected. Continuing normal boot.");
                return false;
            }
            _soc.getSleeper().sleep(10); // yield
        }

        _soc.getLogger().i(TAG, "No button press detected. Continuing normal boot.");
        return false;
    }

    void AviatorClock::_startNormalOperation()
    {
        if (_secondHand)
        {
            _secondHand->start();
            _soc.getLogger().i(TAG, "ClockHand task started.");
        }

        // Start the background task for button monitoring
        _buttonTask = _soc.createTask(
            [this]()
            { this->buttonMonitoringTask(); },
            "button_task");
        _soc.getLogger().i(TAG, "Button monitoring task started.");
    }

    void AviatorClock::_enterWifiSetupMode()
    {
        // Here we will eventually call the WiFi setup function
        // For now, we'll just indicate it and halt execution
        _soc.getLogger().i(TAG, "Setup mode activated. Further implementation needed.");
        while (true)
        {
            _soc.getSleeper().sleep(1000);
        }
    }


    void AviatorClock::buttonMonitoringTask()
    {
        _soc.getLogger().i(TAG, "Button monitoring task running.");
        while (true)
        {
            _setupButton->update();
            if (_setupButton->wasLongPressed())
            {
                _soc.getLogger().w(TAG, "Long press detected during operation. Restarting to enter setup mode...");
                _soc.getSleeper().sleep(1000); // Wait a moment for the log to be sent
                ESP.restart();
            }
            _soc.getSleeper().sleep(20); // Check every 20ms
        }
    }

} // namespace aviator_clock
