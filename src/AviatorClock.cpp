#include "AviatorClock.h"
#include "esp_log.h"

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
    AviatorClock::AviatorClock(soc::api::ISoc &soc) : _soc(soc)
    {
        ESP_LOGI(TAG, "AviatorClock instance created.");
    }

    AviatorClock::~AviatorClock()
    {
        ESP_LOGI(TAG, "AviatorClock instance destroyed.");
    }

    void AviatorClock::setup()
    {
        ESP_LOGI(TAG, "Setting up AviatorClock components...");

        // Create hardware-related components using the encapsulated constants
        _hallSensor = _soc.createDigitalInput(HALL_SENSOR_PIN, true /* isActiveLow */);
        _hallSensor->begin();
        _setupButtonPin = _soc.createDigitalInput(SETUP_BUTTON_PIN, true /* isActiveLow */);
        _setupButtonPin->begin();
        _setupButton = std::make_unique<LongPressButton>(_soc, *_setupButtonPin);

        _accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
        ESP_LOGI(TAG, "Level 0 components created.");

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
            *_accelWrapper,
            *_hallSensor,
            homingConfig);

        ESP_LOGI(TAG, "Level 1 components (HomingStrategy) created.");

        // The enable pin is now passed to the constructor to allow for software control.
        auto motor = std::make_unique<stepper::accel::AccelStepperMotor>(
            *_accelWrapper,
            EFFECTIVE_STEPS_PER_REVOLUTION,
            *_homingStrategy,
            ENABLE_PIN_HW);

        ESP_LOGI(TAG, "Level 2 components (Motor) created.");

        // Create the main application component (ClockHand)
        _secondHand = std::make_unique<aviator_clock::ClockHand>(
            aviator_clock::ClockHand::HandType::SECOND,
            _soc,
            std::move(motor),
            DIAL_TOTAL_ACTIVE_ANGLE,
            SHARP_TICK_SPEED_DPS,
            SHARP_TICK_ACCELERATION_DPS2);

        ESP_LOGI(TAG, "Level 3 components (ClockHand) created.");

        // Setup the ClockHand itself
        if (_secondHand)
        {
            _secondHand->setup();
        }
    }

    void AviatorClock::start()
    {
        ESP_LOGI(TAG, "Starting AviatorClock logic...");

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
        ESP_LOGI(TAG, "Checking for setup mode activation...");
        ESP_LOGI(TAG, "Hold button on GPIO%d for 3s to enter setup.", SETUP_BUTTON_PIN);

        const unsigned long checkDuration = 4000; // ms
        unsigned long checkStartTime = _soc.getTimeProvider().millis();

        while (_soc.getTimeProvider().millis() - checkStartTime < checkDuration)
        {
            _setupButton->update();
            if (_setupButton->wasLongPressed())
            {
                ESP_LOGW(TAG, "Long press detected! Entering setup mode.");
                return true;
            }
            if (_setupButton->wasShortPressed())
            {
                ESP_LOGI(TAG, "Short press detected. Continuing normal boot.");
                return false;
            }
            _soc.getSleeper().sleep(10); // yield
        }

        ESP_LOGI(TAG, "No button press detected. Continuing normal boot.");
        return false;
    }

    void AviatorClock::_startNormalOperation()
    {
        if (_secondHand)
        {
            _secondHand->start();
            ESP_LOGI(TAG, "ClockHand task started.");
        }

        // Start the background task for button monitoring
        _buttonTask = _soc.createTask(
            [this]()
            { this->buttonMonitoringTask(); },
            "button_task");
        ESP_LOGI(TAG, "Button monitoring task started.");
    }

    void AviatorClock::_enterWifiSetupMode()
    {
        // Here we will eventually call the WiFi setup function
        // For now, we'll just indicate it and halt execution
        ESP_LOGI(TAG, "Setup mode activated. Further implementation needed.");
        while (true)
        {
            _soc.getSleeper().sleep(1000);
        }
    }


    void AviatorClock::buttonMonitoringTask()
    {
        ESP_LOGI(TAG, "Button monitoring task running.");
        while (true)
        {
            _setupButton->update();
            if (_setupButton->wasLongPressed())
            {
                ESP_LOGW(TAG, "Long press detected during operation. Restarting to enter setup mode...");
                _soc.getSleeper().sleep(1000); // Wait a moment for the log to be sent
                ESP.restart();
            }
            _soc.getSleeper().sleep(20); // Check every 20ms
        }
    }

} // namespace aviator_clock
